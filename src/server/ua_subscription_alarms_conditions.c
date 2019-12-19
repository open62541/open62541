/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Sameer AL-Qadasi)
 */

/*****************************************************************************/
/* Include Files Required                                                    */
/*****************************************************************************/
#include "ua_server_internal.h"
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

/*****************************************************************************/
/* defines                                                                 */
/*****************************************************************************/
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
/* Prototypes                                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Callbacks                                                                 */
/*****************************************************************************/

/**
 * Function used to set a user specific callback to TwoStateVariable Fields of
 * a condition. The callbacks will be called before triggering the events when
 * transition to true State of EnabledState/Id, AckedState/Id, ConfirmedState/Id
 * and ActiveState/Id occurs.
 * @param removeBranch is not used for the first implementation
 */
UA_StatusCode
UA_Server_setConditionTwoStateVariableCallback(UA_Server *server, const UA_NodeId condition,
                                               const UA_NodeId conditionSource, UA_Boolean removeBranch,
                                               UA_TwoStateVariableChangeCallback callback,
                                               UA_TwoStateVariableCallbackType callbackType) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, &conditionSource))
            continue;

        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(!UA_NodeId_equal(&conditionEntryTmp->conditionId, &condition))
                continue;

            switch(callbackType) {
                case UA_ENTERING_ENABLEDSTATE:
                    conditionEntryTmp->specificCallbacksData.enteringEnabledStateCallback = callback;
                    return UA_STATUSCODE_GOOD;

                case UA_ENTERING_ACKEDSTATE:
                    conditionEntryTmp->specificCallbacksData.enteringAckedStateCallback = callback;
                    conditionEntryTmp->specificCallbacksData.ackedRemoveBranch = removeBranch;
                    return UA_STATUSCODE_GOOD;

                case UA_ENTERING_CONFIRMEDSTATE:
                    conditionEntryTmp->specificCallbacksData.enteringConfirmedStateCallback = callback;
                    conditionEntryTmp->specificCallbacksData.confirmedRemoveBranch = removeBranch;
                    return UA_STATUSCODE_GOOD;

                case UA_ENTERING_ACTIVESTATE:
                    conditionEntryTmp->specificCallbacksData.enteringActiveStateCallback = callback;
                    return UA_STATUSCODE_GOOD;

                default:
                    return UA_STATUSCODE_BADNOTFOUND;
            }
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
getConditionTwoStateVariableCallback(UA_Server *server,
                                    const UA_NodeId *branch,
                                    UA_Condition_nodeListElement *conditionEntry,
                                    UA_Boolean *removeBranch,
                                    UA_TwoStateVariableCallbackType callbackType) {
    switch(callbackType) {
        case UA_ENTERING_ENABLEDSTATE:
            if(conditionEntry->specificCallbacksData.enteringEnabledStateCallback != NULL) {
              return conditionEntry->specificCallbacksData.enteringEnabledStateCallback(server, branch);
            }
            return UA_STATUSCODE_GOOD;//TODO log warning when the callback wasn't set

        case UA_ENTERING_ACKEDSTATE:
            if(conditionEntry->specificCallbacksData.enteringAckedStateCallback != NULL) {
              *removeBranch = conditionEntry->specificCallbacksData.ackedRemoveBranch;
              return conditionEntry->specificCallbacksData.enteringAckedStateCallback(server, branch);
            }
            return UA_STATUSCODE_GOOD;

        case UA_ENTERING_CONFIRMEDSTATE:
            if(conditionEntry->specificCallbacksData.enteringConfirmedStateCallback != NULL) {
              *removeBranch = conditionEntry->specificCallbacksData.confirmedRemoveBranch;
              return conditionEntry->specificCallbacksData.enteringConfirmedStateCallback(server, branch);
            }
            return UA_STATUSCODE_GOOD;

        case UA_ENTERING_ACTIVESTATE:
            if(conditionEntry->specificCallbacksData.enteringActiveStateCallback != NULL) {
              return conditionEntry->specificCallbacksData.enteringActiveStateCallback(server, branch);
            }
            return UA_STATUSCODE_GOOD;

        default:
            return UA_STATUSCODE_BADNOTFOUND;
      }
}

static UA_StatusCode
callConditionTwoStateVariableCallback(UA_Server *server,
                                      const UA_NodeId *condition,
                                      const UA_NodeId *conditionSource,
                                      UA_Boolean *removeBranch,
                                      UA_TwoStateVariableCallbackType callbackType) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(UA_NodeId_equal(&conditionEntryTmp->conditionId, condition)) {
                return getConditionTwoStateVariableCallback(server, condition, conditionEntryTmp, removeBranch, callbackType);
            }
            else {
                UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
                LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                    if(conditionBranchEntryTmp->conditionBranchId != NULL &&
                       UA_NodeId_equal(conditionBranchEntryTmp->conditionBranchId, condition))
                        return getConditionTwoStateVariableCallback(server, conditionBranchEntryTmp->conditionBranchId,
                                                                 conditionEntryTmp, removeBranch, callbackType);
                }
            }
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

/**
 * Gets the parent NodeId of a Field (e.g. Severity) or Field Property (e.g. EnabledState/Id)
 */
static UA_StatusCode
getFieldParentNodeId(UA_Server *server, const UA_NodeId *field, UA_NodeId *parent) {
    *parent = UA_NODEID_NULL;
    UA_NodeId hasPropertyType = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    UA_NodeId hasComponentType = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    const UA_VariableNode *fieldNode = (const UA_VariableNode *)UA_NODESTORE_GET(server, field);
    if(fieldNode != NULL) {
        for(size_t i = 0; i < fieldNode->referencesSize; i++) {
            if((UA_NodeId_equal(&fieldNode->references[i].referenceTypeId, &hasPropertyType) ||
               UA_NodeId_equal(&fieldNode->references[i].referenceTypeId, &hasComponentType)) &&
               (true == fieldNode->references[i].isInverse)) {
                UA_StatusCode retval = UA_NodeId_copy(&fieldNode->references[i].refTargets->target.nodeId, parent);
                UA_NODESTORE_RELEASE(server, (const UA_Node *)fieldNode);
                return retval;
            }
        }
    }

    if(fieldNode != NULL)
        UA_NODESTORE_RELEASE(server, (const UA_Node *)fieldNode);

    return UA_STATUSCODE_BADNOTFOUND;
}

/**
 * Gets the NodeId of a Field (e.g. Severity)
 */
static UA_StatusCode
getConditionFieldNodeId(UA_Server *server,
                        const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName,
                        UA_NodeId *outFieldNodeId) {
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *conditionNodeId, 1, fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD) {
        return bpr.statusCode;
    }

    UA_StatusCode retval = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, outFieldNodeId);
    UA_BrowsePathResult_deleteMembers(&bpr);

    return retval;
}

/**
 * Gets the NodeId of a Field Property (e.g. EnabledState/Id)
 */
static UA_StatusCode
getConditionFieldPropertyNodeId(UA_Server *server,
                                const UA_NodeId *originCondition,
                                const UA_QualifiedName* variableFieldName,
                                const UA_QualifiedName* variablePropertyName,
                                UA_NodeId *outFieldPropertyNodeId) {
    /* 1) Find Variable Field of the Condition */
    UA_BrowsePathResult bprConditionVariableField = UA_Server_browseSimplifiedBrowsePath(server, *originCondition, 1, variableFieldName);
    if(bprConditionVariableField.statusCode != UA_STATUSCODE_GOOD) {
        return bprConditionVariableField.statusCode;
    }
    /* 2) Find Property of the Variable Field of the Condition */
    UA_BrowsePathResult bprVariableFieldProperty = UA_Server_browseSimplifiedBrowsePath(server, bprConditionVariableField.targets->targetId.nodeId,
                                                                                        1, variablePropertyName);
    if(bprVariableFieldProperty.statusCode != UA_STATUSCODE_GOOD) {
        UA_BrowsePathResult_deleteMembers(&bprConditionVariableField);
        return bprVariableFieldProperty.statusCode;
    }

    UA_StatusCode retval = UA_NodeId_copy(&bprVariableFieldProperty.targets[0].targetId.nodeId, outFieldPropertyNodeId);

    UA_BrowsePathResult_deleteMembers(&bprConditionVariableField);
    UA_BrowsePathResult_deleteMembers(&bprVariableFieldProperty);

    return retval;
}

/**
 * Gets NodeId value of a Field which has NodeId as DataType (e.g. EventType)
 */
static UA_StatusCode
getNodeIdValueOfConditionField(UA_Server *server,
                               const UA_NodeId *condition,
                               UA_QualifiedName fieldName,
                               UA_NodeId *outNodeId) {
    *outNodeId = UA_NODEID_NULL;
    UA_NodeId nodeIdValue;
    UA_StatusCode retval = getConditionFieldNodeId(server, condition,
                                                   &fieldName, &nodeIdValue);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Field not found",);

    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);

    /* Read the Value of SourceNode Property Node (the Value is a NodeId) */
    retval = UA_Server_readValue(server, nodeIdValue, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_NodeId_deleteMembers(&nodeIdValue);
        UA_Variant_deleteMembers(&tOutVariant);
        return retval;
    }

    retval = UA_NodeId_copy((UA_NodeId*)tOutVariant.data, outNodeId);

    UA_NodeId_deleteMembers(&nodeIdValue);
    UA_Variant_deleteMembers(&tOutVariant);

    return retval;
}

/**
 * Gets the NodeId of a condition branch. In case of main branch (BranchId == NULL),
 * ConditionId will be returned.
 */
static UA_StatusCode
getConditionBranchNodeId(UA_Server *server,
                   const UA_ByteString *eventId,
                   UA_NodeId *outConditionBranchNodeId) {
    *outConditionBranchNodeId = UA_NODEID_NULL;
    /* The function checks the BranchId based on the event Id, if BranchId == NULL -> outConditionId = ConditionId */
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            /* Get Branch Entry*/
            UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
            LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                if(!UA_ByteString_equal(&conditionBranchEntryTmp->lastEventId, eventId))
                    continue;
                if(conditionBranchEntryTmp->conditionBranchId == NULL)
                    return UA_NodeId_copy(&conditionEntryTmp->conditionId, outConditionBranchNodeId);
                else
                    return UA_NodeId_copy(conditionBranchEntryTmp->conditionBranchId, outConditionBranchNodeId);
            }
        }
    }

    return UA_STATUSCODE_BADEVENTIDUNKNOWN;
}

static UA_StatusCode
getConditionLastSeverity(UA_Server *server,
                         const UA_NodeId *conditionSource,
                         const UA_NodeId *conditionId,
                         UA_LastSverity_Data *outLastSeverity) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(!UA_NodeId_equal(&conditionEntryTmp->conditionId, conditionId))
                continue;
            outLastSeverity->lastSeverity = conditionEntryTmp->lastSevertyData.lastSeverity;
            outLastSeverity->sourceTimeStamp = conditionEntryTmp->lastSevertyData.sourceTimeStamp;
            return UA_STATUSCODE_GOOD;
        }
    }

    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Entry not found in list!");
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
updateConditionLastSeverity(UA_Server *server,
                            const UA_NodeId *conditionSource,
                            const UA_NodeId *conditionId,
                            const UA_LastSverity_Data *outLastSeverity) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(!UA_NodeId_equal(&conditionEntryTmp->conditionId, conditionId))
                continue;
            conditionEntryTmp->lastSevertyData.lastSeverity = outLastSeverity->lastSeverity;
            conditionEntryTmp->lastSevertyData.sourceTimeStamp =  outLastSeverity->sourceTimeStamp;
            return UA_STATUSCODE_GOOD;
        }
    }

    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Entry not found in list!");
    return UA_STATUSCODE_BADNOTFOUND;
}


static UA_StatusCode
getConditionActiveState(UA_Server *server,
                         const UA_NodeId *conditionSource,
                         const UA_NodeId *conditionId,
                         UA_ActiveState *outLastActiveState,
                         UA_ActiveState *outCurrentActiveState,
                         UA_Boolean *outIsLimitAlarm) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(!UA_NodeId_equal(&conditionEntryTmp->conditionId, conditionId))
                continue;
            *outLastActiveState = conditionEntryTmp->lastActiveState;
            *outCurrentActiveState = conditionEntryTmp->currentActiveState;
            *outIsLimitAlarm = conditionEntryTmp->isLimitAlarm;
            return UA_STATUSCODE_GOOD;
        }
    }

    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Entry not found in list!");
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
updateConditionActiveState(UA_Server *server,
                            const UA_NodeId *conditionSource,
                            const UA_NodeId *conditionId,
                            const UA_ActiveState lastActiveState,
                            const UA_ActiveState currentActiveState,
                            UA_Boolean isLimitAlarm) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(!UA_NodeId_equal(&conditionEntryTmp->conditionId, conditionId))
                continue;
            conditionEntryTmp->lastActiveState = lastActiveState;
            conditionEntryTmp->currentActiveState = currentActiveState;
            conditionEntryTmp->isLimitAlarm = isLimitAlarm;
            return UA_STATUSCODE_GOOD;
        }
    }

    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Entry not found in list!");
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
updateConditionLastEventId(UA_Server *server,
                           const UA_NodeId *triggeredEvent,
                           const UA_NodeId *ConditionSource,
                           const UA_ByteString *lastEventId) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, ConditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(UA_NodeId_equal(&conditionEntryTmp->conditionId, triggeredEvent)) {// found ConditionId -> branch == NULL
                UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
                LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                    if(conditionBranchEntryTmp->conditionBranchId == NULL) { // update main condition branch
                        UA_ByteString_deleteMembers(&conditionBranchEntryTmp->lastEventId);
                        return UA_ByteString_copy(lastEventId, &conditionBranchEntryTmp->lastEventId);
                    }
                }
            }
            else { // TODO update condition branch
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,"Condition Branch not implemented");
                return UA_STATUSCODE_BADNOTFOUND;
            }
        }
    }

    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Entry not found in list!");
    return UA_STATUSCODE_BADNOTFOUND;
}

static void
setIsCallerAC(UA_Server *server,
              const UA_NodeId *condition,
              const UA_NodeId *conditionSource,
              UA_Boolean isCallerAC) {
    /* Get conditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(UA_NodeId_equal(&conditionEntryTmp->conditionId, condition)) {// found ConditionId -> branch == NULL
                UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
                LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                    if(conditionBranchEntryTmp->conditionBranchId == NULL) {
                        conditionBranchEntryTmp->isCallerAC = isCallerAC;
                        return;
                    }
                }
            }
            else {// TODO condition branch
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Condition Branch not implemented");
                return;
            }
        }
    }

    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Entry not found in list!");
}

UA_Boolean
isConditionOrBranch(UA_Server *server,
                    const UA_NodeId *condition,
                    const UA_NodeId *conditionSource,
                    UA_Boolean *isCallerAC) {
    /* Get conditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(UA_NodeId_equal(&conditionEntryTmp->conditionId, condition)) {// found ConditionId -> branch == NULL
                UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
                    LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                        if(conditionBranchEntryTmp->conditionBranchId == NULL) {
                          *isCallerAC = conditionBranchEntryTmp->isCallerAC;
                          return true;
                        }
                    }
            }
            else {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Condition Branch not implemented");
                return false;
            }
        }
    }

    return false;
}

static UA_Boolean
isRetained(UA_Server *server, const UA_NodeId *condition) {
    UA_QualifiedName retain = UA_QUALIFIEDNAME(0, CONDITION_FIELD_RETAIN);
    UA_NodeId retainNodeId;

    /* Get EnabledStateId NodeId */
    UA_StatusCode retval = getConditionFieldNodeId(server, condition, &retain, &retainNodeId);
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
          UA_NodeId_deleteMembers(&retainNodeId);
          return false;
    }

    if(*(UA_Boolean *)tOutVariant.data == true) {
        UA_NodeId_deleteMembers(&retainNodeId);
        UA_Variant_deleteMembers(&tOutVariant);
        return true;
    }

    UA_NodeId_deleteMembers(&retainNodeId);
    UA_Variant_deleteMembers(&tOutVariant);
    return false;
}

static UA_Boolean
isTwoStateVariableInTrueState(UA_Server *server,
                              const UA_NodeId *condition,
                              UA_QualifiedName *twoStateVariable) {
    UA_QualifiedName twoStateVariableId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_NodeId twoStateVariableIdNodeId;

    /* Get TwoStateVariableId NodeId */
    UA_StatusCode retval = getConditionFieldPropertyNodeId(server, condition, twoStateVariable,
                                                           &twoStateVariableId, &twoStateVariableIdNodeId);
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
        UA_NodeId_deleteMembers(&twoStateVariableIdNodeId);
        return false;
    }

    UA_NodeId_deleteMembers(&twoStateVariableIdNodeId);

    if(*(UA_Boolean *)tOutVariant.data == true) {
      UA_Variant_deleteMembers(&tOutVariant);
      return true;
    }

    UA_Variant_deleteMembers(&tOutVariant);
    return false;
}

static UA_StatusCode
enteringDisabledState(UA_Server *server,
                      const UA_NodeId *conditionId,
                      const UA_NodeId *conditionSource) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldMessage = UA_QUALIFIEDNAME(0, CONDITION_FIELD_MESSAGE);
    UA_QualifiedName fieldRetain = UA_QUALIFIEDNAME(0, CONDITION_FIELD_RETAIN);
    UA_Boolean retain = false;
    UA_LocalizedText message;
    UA_LocalizedText enableText;
    UA_NodeId triggeredNode;
    UA_Variant value;

    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(!UA_NodeId_equal(&conditionEntryTmp->conditionId, conditionId))
                continue;
            /* Get Branch Entry*/
            UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
            LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                UA_NodeId_init(&triggeredNode);
                if(conditionBranchEntryTmp->conditionBranchId == NULL) //disable main Condition Branch (BranchId == NULL)
                    triggeredNode = conditionEntryTmp->conditionId;
                else //disable all branches
                    triggeredNode = *(conditionBranchEntryTmp->conditionBranchId);

                message = UA_LOCALIZEDTEXT(LOCALE, DISABLED_MESSAGE);
                enableText = UA_LOCALIZEDTEXT(LOCALE, DISABLED_TEXT);
                UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
                UA_StatusCode retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldMessage);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Message failed",);

                UA_Variant_setScalar(&value, &enableText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
                retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldEnabledState);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition EnabledState text failed",);

                UA_Variant_setScalar(&value, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
                retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldRetain);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Retain failed",);

                /* Trigger event */
                UA_ByteString lastEventId = UA_BYTESTRING_NULL;
                /* Trigger the event for Condition or its Branch */
                setIsCallerAC(server, &triggeredNode, conditionSource, true);
                retval = UA_Server_triggerEvent(server, triggeredNode, conditionSourceEntryTmp->conditionSourceId,
                                                &lastEventId, false); //Condition Nodes should not be deleted after triggering the event
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);
                setIsCallerAC(server, &triggeredNode, conditionSource, false);

                /* Update list */
                retval = updateConditionLastEventId(server, &triggeredNode, &conditionSourceEntryTmp->conditionSourceId, &lastEventId);
                UA_ByteString_deleteMembers(&lastEventId);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "updating condition event failed",);
            }

            return UA_STATUSCODE_GOOD;
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
enteringEnabledState(UA_Server *server,
                     const UA_NodeId *conditionId,
                     const UA_NodeId *conditionSource) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldMessage = UA_QUALIFIEDNAME(0, CONDITION_FIELD_MESSAGE);
    UA_LocalizedText message;
    UA_LocalizedText enableText;
    UA_NodeId triggeredNode;
    UA_Variant value;

    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        if(!UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(!UA_NodeId_equal(&conditionEntryTmp->conditionId, conditionId))
                continue;
            /* Get Branch Entry*/
            UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
            LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                UA_NodeId_init(&triggeredNode);
                if(conditionBranchEntryTmp->conditionBranchId == NULL) //enable main Condition
                    triggeredNode = conditionEntryTmp->conditionId;
                else //enable branches
                    triggeredNode = *(conditionBranchEntryTmp->conditionBranchId);

                message = UA_LOCALIZEDTEXT(LOCALE, ENABLED_MESSAGE);
                enableText = UA_LOCALIZEDTEXT(LOCALE, ENABLED_TEXT);
                UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
                UA_StatusCode retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldMessage);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "set Condition Message failed",);

                UA_Variant_setScalar(&value, &enableText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
                retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldEnabledState);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "set Condition EnabledState text failed",);

                /* User callback TODO how should branches be evaluated? see p.19 (5.5.2) */
                UA_Boolean removeBranch = false;//not used
                retval = callConditionTwoStateVariableCallback(server, &triggeredNode, conditionSource, &removeBranch, UA_ENTERING_ENABLEDSTATE);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "calling condition callback failed",);

                /* Trigger event */
                retval = UA_Server_triggerConditionEvent(server, triggeredNode, *conditionSource,
                                                         NULL); //Condition Nodes should not be deleted after triggering the event
                CONDITION_ASSERT_RETURN_RETVAL(retval, "triggering condition event failed",);
            }

            return UA_STATUSCODE_GOOD;
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;
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
    UA_NodeId_deleteMembers(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given EnabledState",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENODE), &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found", UA_NodeId_deleteMembers(&conditionNode););

    /* Set disabling/enabling time */
    retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                         (const UA_DateTime*)&data->sourceTimestamp, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set enabling/disabling Time failed", UA_NodeId_deleteMembers(&conditionNode); UA_NodeId_deleteMembers(&conditionSource););

    if(false == (*((UA_Boolean *)data->value.data))) {
        /* Disable all branches and update list */
        retval = enteringDisabledState(server, &conditionNode, &conditionSource);
        UA_NodeId_deleteMembers(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Entering disabled state failed", UA_NodeId_deleteMembers(&conditionSource););
    }
    else {
        /* Enable all branches and update list */
        retval = enteringEnabledState(server, &conditionNode, &conditionSource);
        UA_NodeId_deleteMembers(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Entering enabled state failed", UA_NodeId_deleteMembers(&conditionSource););
    }

    UA_NodeId_deleteMembers(&conditionSource);
}

static void
afterWriteCallbackAckedStateChange(UA_Server *server,
                                   const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *nodeId, void *nodeContext,
                                   const UA_NumericRange *range, const UA_DataValue *data) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldAckedState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ACKEDSTATE);
    UA_QualifiedName fieldAckedStateId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_QualifiedName fieldMessage = UA_QUALIFIEDNAME(0, CONDITION_FIELD_MESSAGE);
    UA_LocalizedText message;
    UA_LocalizedText text;
    UA_Variant value;

    UA_NodeId twoStateVariableNode;
    /* Get the AckedState NodeId then The Condition NodeId */
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent TwoStateVariable found for given AckedState/Id",);

    UA_NodeId conditionNode;
    retval = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_deleteMembers(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given AckedState",);

    /* Callback for change to true in AckedState/Id property.
     * First check whether the value is true (ackedState/Id == true).
     * That check makes it possible to set ackedState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == true) {

        /* Check if enabled and retained */
        if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledState) &&
           isRetained(server, &conditionNode)) {
            /* Set acknowledging time */
            retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                                 &data->sourceTimestamp, &UA_TYPES[UA_TYPES_DATETIME]);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Acknowledging Time failed", UA_NodeId_deleteMembers(&conditionNode););

            /* Set Message */
            message = UA_LOCALIZEDTEXT(LOCALE, ACKED_MESSAGE);
            UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, conditionNode, &value, fieldMessage);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Message failed", UA_NodeId_deleteMembers(&conditionNode););

            /* Set AckedState text */
            text = UA_LOCALIZEDTEXT(LOCALE, ACKED_TEXT);
            UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, conditionNode, &value, fieldAckedState);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition AckedState failed", UA_NodeId_deleteMembers(&conditionNode););

            /* Get conditionSource */
            UA_NodeId conditionSource;
            retval = getNodeIdValueOfConditionField(server, &conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENODE), &conditionSource);
            CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found", UA_NodeId_deleteMembers(&conditionNode););

            /* User callback*/
            UA_Boolean removeBranch = false;
            retval = callConditionTwoStateVariableCallback(server, &conditionNode, &conditionSource, &removeBranch, UA_ENTERING_ACKEDSTATE);
            CONDITION_ASSERT_RETURN_VOID(retval, "Calling condition callback failed", UA_NodeId_deleteMembers(&conditionNode);
                                         UA_NodeId_deleteMembers(&conditionSource););

            /* Trigger event */
            retval = UA_Server_triggerConditionEvent(server, conditionNode, conditionSource,
                                                     NULL); //Condition Nodes should not be deleted after triggering the event
            CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed", UA_NodeId_deleteMembers(&conditionNode);
                                         UA_NodeId_deleteMembers(&conditionSource););
        }
        else {
            /* Set AckedState/Id to false*/
            UA_Boolean idValue = false;
            UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value, fieldAckedState, fieldAckedStateId);
            UA_NodeId_deleteMembers(&conditionNode);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set AckedState/Id failed",);
        }
    }
    else {
        /* Set unacknowledging time */
        retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                             &data->sourceTimestamp, &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set deactivating Time failed", UA_NodeId_deleteMembers(&conditionNode););

        /* Set AckedState text to Unacknowledged*/
        text = UA_LOCALIZEDTEXT(LOCALE, UNACKED_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, conditionNode, &value, fieldAckedState);
        UA_NodeId_deleteMembers(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition AckedState failed",);
    }
}

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT

static void
afterWriteCallbackConfirmedStateChange(UA_Server *server,
                                       const UA_NodeId *sessionId, void *sessionContext,
                                       const UA_NodeId *nodeId, void *nodeContext,
                                       const UA_NumericRange *range, const UA_DataValue *data) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldConfirmedState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_CONFIRMEDSTATE);
    UA_QualifiedName fieldConfirmedStateId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_QualifiedName fieldMessage = UA_QUALIFIEDNAME(0, CONDITION_FIELD_MESSAGE);
    UA_LocalizedText message;
    UA_LocalizedText text;
    UA_Variant value;

    UA_NodeId twoStateVariableNode;
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent TwoStateVariable found for given ConfirmedState/Id",);

    UA_NodeId conditionNode;
    retval = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_deleteMembers(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given ConfirmedState",);

    /* Callback to change to true in ConfirmedState/Id property.
     * First check whether the value is true (ConfirmedState/Id == true).
     * That check makes it possible to set ConfirmedState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == true) {

        /* Check if enabled and retained */
        if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledState) &&
            isRetained(server, &conditionNode)) {
            /* Set confirming time */
            retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                                 &data->sourceTimestamp, &UA_TYPES[UA_TYPES_DATETIME]);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Confirming Time failed", UA_NodeId_deleteMembers(&conditionNode););

            /* Set Message */
            message = UA_LOCALIZEDTEXT(LOCALE, CONFIRMED_MESSAGE);
            UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, conditionNode, &value, fieldMessage);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Message failed", UA_NodeId_deleteMembers(&conditionNode););

            /* Set ConfirmedState text */
            text = UA_LOCALIZEDTEXT(LOCALE, CONFIRMED_TEXT);
            UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, conditionNode, &value, fieldConfirmedState);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ConfirmedState failed", UA_NodeId_deleteMembers(&conditionNode););

            /* Get conditionSource */
            UA_NodeId conditionSource;
            retval = getNodeIdValueOfConditionField(server, &conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENODE), &conditionSource);
            CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found", UA_NodeId_deleteMembers(&conditionNode););

            /* User callback*/
            UA_Boolean removeBranch = false;
            retval = callConditionTwoStateVariableCallback(server, &conditionNode, &conditionSource, &removeBranch, UA_ENTERING_CONFIRMEDSTATE);
            CONDITION_ASSERT_RETURN_VOID(retval, "Calling condition callback failed", UA_NodeId_deleteMembers(&conditionNode);
                                         UA_NodeId_deleteMembers(&conditionSource););

            /* Trigger event */
            retval = UA_Server_triggerConditionEvent(server, conditionNode, conditionSource,
                                                     NULL); //Condition Nodes should not be deleted after triggering the event
            CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed", UA_NodeId_deleteMembers(&conditionNode);
                                         UA_NodeId_deleteMembers(&conditionSource););
        }
        else {
            /* Set confirmedState/Id to false*/
            UA_Boolean idValue = false;
            UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value, fieldConfirmedState, fieldConfirmedStateId);
            UA_NodeId_deleteMembers(&conditionNode);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set ConfirmedState/Id failed",);
        }
    }
    else {
        /* Set unconfirming time */
        retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                             &data->sourceTimestamp, &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set deactivating Time failed", UA_NodeId_deleteMembers(&conditionNode););

        /* Set ConfirmedState text to (Unconfirmed)*/
        text = UA_LOCALIZEDTEXT(LOCALE, UNCONFIRMED_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, conditionNode, &value, fieldConfirmedState);
        UA_NodeId_deleteMembers(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ConfirmedState failed",);
    }
}
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

static void
afterWriteCallbackActiveStateChange(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *nodeId, void *nodeContext,
                                    const UA_NumericRange *range, const UA_DataValue *data) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldActiveState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ACTIVESTATE);
    UA_QualifiedName fieldActiveStateId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_LocalizedText text;
    UA_Variant value;

    UA_NodeId twoStateVariableNode;
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent TwoStateVariable found for given ActiveState/Id",);

    UA_NodeId conditionNode;
    retval = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_deleteMembers(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given ActiveState",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENODE), &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found", UA_NodeId_deleteMembers(&conditionNode););

    UA_ActiveState lastActiveState = UA_INACTIVE, currentActiveState = UA_INACTIVE;
    UA_Boolean isLimitalarm = false;

    retval = getConditionActiveState(server, &conditionSource, &conditionNode, &lastActiveState, &currentActiveState, &isLimitalarm);
    CONDITION_ASSERT_RETURN_VOID(retval, "ActiveState transition check failed", UA_NodeId_deleteMembers(&conditionNode);
                                 UA_NodeId_deleteMembers(&conditionSource););

    if(isLimitalarm == false) {
      if(*((UA_Boolean *)data->value.data) == true) {
        retval = updateConditionActiveState(server, &conditionSource, &conditionNode, currentActiveState, UA_ACTIVE, false);
        CONDITION_ASSERT_RETURN_VOID(retval, "Updating ActiveState failed", UA_NodeId_deleteMembers(&conditionNode);
                                     UA_NodeId_deleteMembers(&conditionSource););
      }
      else {
        retval = updateConditionActiveState(server, &conditionSource, &conditionNode, currentActiveState, UA_INACTIVE, false);
        CONDITION_ASSERT_RETURN_VOID(retval, "Updating ActiveState failed", UA_NodeId_deleteMembers(&conditionNode);
                                     UA_NodeId_deleteMembers(&conditionSource););
      }

      retval = getConditionActiveState(server, &conditionSource, &conditionNode, &lastActiveState, &currentActiveState, &isLimitalarm);
      CONDITION_ASSERT_RETURN_VOID(retval, "ActiveState transition check failed", UA_NodeId_deleteMembers(&conditionNode);
                                     UA_NodeId_deleteMembers(&conditionSource););
    }

    /* callback for change to true in ActiveState/Id property.
     * first check whether the value is true (ActiveState/Id == true).
     * That check makes it possible to set ActiveState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == true &&
       (lastActiveState != currentActiveState)) {

        /* Check if enabled and retained */
        if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledState) &&
            isRetained(server, &conditionNode)) {
            /* Set activating time */
            retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                                 &data->sourceTimestamp, &UA_TYPES[UA_TYPES_DATETIME]);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set activating Time failed", UA_NodeId_deleteMembers(&conditionNode);
                                         UA_NodeId_deleteMembers(&conditionSource););

            /* Set ActiveState text */
            text = UA_LOCALIZEDTEXT(LOCALE, ACTIVE_TEXT);
            UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, conditionNode, &value, fieldActiveState);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ActiveState failed", UA_NodeId_deleteMembers(&conditionNode);
                                         UA_NodeId_deleteMembers(&conditionSource););

            /* User callback*/
            UA_Boolean removeBranch = false;//not used
            retval = callConditionTwoStateVariableCallback(server, &conditionNode, &conditionSource, &removeBranch, UA_ENTERING_ACTIVESTATE);
            CONDITION_ASSERT_RETURN_VOID(retval, "Calling condition callback failed", UA_NodeId_deleteMembers(&conditionNode);
                                         UA_NodeId_deleteMembers(&conditionSource););

            /* Trigger event */
            retval = UA_Server_triggerConditionEvent(server, conditionNode, conditionSource,
                                                    NULL); //Condition Nodes should not be deleted after triggering the event
            UA_NodeId_deleteMembers(&conditionNode);
            UA_NodeId_deleteMembers(&conditionSource);
            CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed",);
        }
        else {
            /* Set ActiveState/Id to false -> don't apply changes in case of disabled or not retained*/
            UA_Boolean idValue = false;
            UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value, fieldActiveState, fieldActiveStateId);
            UA_NodeId_deleteMembers(&conditionSource);
            UA_NodeId_deleteMembers(&conditionNode);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set ActiveState/Id failed",);
        }
    }
    else if((*((UA_Boolean *)data->value.data) == false) &&
            (lastActiveState != currentActiveState)) {
             /* Set ActiveState text to (Inactive) */
             text = UA_LOCALIZEDTEXT(LOCALE, INACTIVE_TEXT);
             UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
             retval = UA_Server_setConditionField(server, conditionNode, &value, fieldActiveState);
             CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ActiveState failed", UA_NodeId_deleteMembers(&conditionNode);
                                          UA_NodeId_deleteMembers(&conditionSource););

             /* Set deactivating time */
             retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                                  &data->sourceTimestamp, &UA_TYPES[UA_TYPES_DATETIME]);
             CONDITION_ASSERT_RETURN_VOID(retval, "Set deactivating Time failed", UA_NodeId_deleteMembers(&conditionNode);
                                          UA_NodeId_deleteMembers(&conditionSource););

             retval = updateConditionActiveState(server, &conditionSource, &conditionNode, currentActiveState, UA_INACTIVE, isLimitalarm);
             UA_NodeId_deleteMembers(&conditionSource);
             UA_NodeId_deleteMembers(&conditionNode);
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
    UA_QualifiedName fieldSourceTimeStamp = UA_QUALIFIEDNAME(0, CONDITION_FIELD_CONDITIONVARIABLE_SOURCETIMESTAMP);
    UA_QualifiedName fieldMessage = UA_QUALIFIEDNAME(0, CONDITION_FIELD_MESSAGE);
    UA_QualifiedName fieldTime = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME);
    UA_NodeId condition;
    UA_LocalizedText message;
    UA_Variant value;

    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &condition);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given Severity Field",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &condition, UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENODE), &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found", UA_NodeId_deleteMembers(&condition););

    UA_LastSverity_Data outLastSeverity;
    retval = getConditionLastSeverity(server, &conditionSource, &condition, &outLastSeverity);
    CONDITION_ASSERT_RETURN_VOID(retval, "Get Condition LastSeverity failed", UA_NodeId_deleteMembers(&condition);
                                 UA_NodeId_deleteMembers(&conditionSource););

    /* Set message dependent on compare result*/
    if(outLastSeverity.lastSeverity < (*(UA_UInt16 *)data->value.data))
        message = UA_LOCALIZEDTEXT(LOCALE, SEVERITY_INCREASED_MESSAGE);
    else
        message = UA_LOCALIZEDTEXT(LOCALE, SEVERITY_DECREASED_MESSAGE);

    /* Set LastSeverity */
    UA_Variant_setScalar(&value, &outLastSeverity.lastSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_setConditionField(server, condition, &value, fieldLastSeverity);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition LAstSeverity failed", UA_NodeId_deleteMembers(&condition);
                                 UA_NodeId_deleteMembers(&conditionSource););

    /* Set SourceTimestamp */
    UA_Variant_setScalar(&value, &outLastSeverity.sourceTimeStamp, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = UA_Server_setConditionVariableFieldProperty(server, condition, &value, fieldLastSeverity, fieldSourceTimeStamp);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set LastSeverity SourceTimestamp failed", UA_NodeId_deleteMembers(&condition);
                                 UA_NodeId_deleteMembers(&conditionSource););

    /* Update lastSeverity in list */
    outLastSeverity.lastSeverity = *(UA_UInt16 *)data->value.data;
    outLastSeverity.sourceTimeStamp = data->sourceTimestamp;
    retval = updateConditionLastSeverity(server, &conditionSource, &condition, &outLastSeverity);
    CONDITION_ASSERT_RETURN_VOID(retval, "Update Condition LastSeverity failed", UA_NodeId_deleteMembers(&condition);
    UA_NodeId_deleteMembers(&conditionSource););

    /* Set Time (Time of Value Change) */
    UA_Variant_setScalar(&value, (void*)(uintptr_t)((const UA_DateTime*)&data->sourceTimestamp), &UA_TYPES[UA_TYPES_DATETIME]);
    retval = UA_Server_setConditionField(server, condition, &value, fieldTime);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Time failed", UA_NodeId_deleteMembers(&condition););

    /* Set Message */
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, condition, &value, fieldMessage);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Message failed", UA_NodeId_deleteMembers(&condition););

#ifdef CONDITION_SEVERITYCHANGECALLBACK_ENABLE
    /* Check if retained */
    if(isRetained(server, &condition)) {
        /* Trigger event */
        retval = UA_Server_triggerConditionEvent(server, condition, conditionSource,
                                                 NULL); //Condition Nodes should not be deleted after triggering the event
        CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed", UA_NodeId_deleteMembers(&condition);
        UA_NodeId_deleteMembers(&conditionSource););
    }
#endif//CONDITION_SEVERITYCHANGECALLBACK_ENABLE
    UA_NodeId_deleteMembers(&conditionSource);
    UA_NodeId_deleteMembers(&condition);
}

static UA_StatusCode
disableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldEnabledStateId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Check if enabled */
    if(isTwoStateVariableInTrueState(server, objectId, &fieldEnabledState)) {
        /* disable by writing false to EnabledState/Id--> will cause a callback to trigger the new events */
        UA_Boolean idValue = false;
        UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        UA_StatusCode retval = UA_Server_setConditionVariableFieldProperty(server, *objectId, &value, fieldEnabledState, fieldEnabledStateId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Disable Condition failed",);
    }
    else
        return UA_STATUSCODE_BADCONDITIONALREADYDISABLED;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
enableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize,
                     const UA_Variant *input, size_t outputSize,
                     UA_Variant *output) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldEnabledStateId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Check if disabled */
    if(!isTwoStateVariableInTrueState(server, objectId, &fieldEnabledState)) {
        /* enable by writing true to EnabledStateId--> will cause a callback to trigger the new events */
        UA_Boolean idValue = true;
        UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        UA_StatusCode retval = UA_Server_setConditionVariableFieldProperty(server, *objectId, &value, fieldEnabledState, fieldEnabledStateId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Enable Condition failed",);
    }
    else
        return UA_STATUSCODE_BADCONDITIONALREADYENABLED;

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
    UA_QualifiedName fieldSourceTimeStamp = UA_QUALIFIEDNAME(0, CONDITION_FIELD_CONDITIONVARIABLE_SOURCETIMESTAMP);
    UA_QualifiedName fieldMessage = UA_QUALIFIEDNAME(0, CONDITION_FIELD_MESSAGE);
    UA_LocalizedText message;
    UA_NodeId triggerEvent;
    UA_Variant value;
    UA_DateTime fieldSourceTimeStampValue = UA_DateTime_now();

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event.
     * see error code Bad_NodeIdInvalid Table 16 p.22. It should be returned when the
     * Method called is from the ConditionType and not its instance, however
     * in current implementation, methods are only being referenced from their ObjectType Node.
     * Because of that, the correct instance (Condition) will be found through
     * its last EventId */
    UA_StatusCode retval = getConditionBranchNodeId(server, (UA_ByteString *)input[0].data, &triggerEvent);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionId based on EventId not found",);

    /* Check if enabled */
    if(isRetained(server, &triggerEvent)) {
        /* Set SourceTimestamp */
        UA_Variant_setScalar(&value, &fieldSourceTimeStampValue, &UA_TYPES[UA_TYPES_DATETIME]);
        retval = UA_Server_setConditionVariableFieldProperty(server, triggerEvent, &value, fieldComment, fieldSourceTimeStamp);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition EnabledState text failed", UA_NodeId_deleteMembers(&triggerEvent););

        /* Set adding comment time (the same value of SourceTimestamp) */
        retval = UA_Server_writeObjectProperty_scalar(server, triggerEvent, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                             &fieldSourceTimeStampValue, &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set enabling/disabling Time failed", UA_NodeId_deleteMembers(&triggerEvent););

        /* Set Message */
        message = UA_LOCALIZEDTEXT(LOCALE, COMMENT_MESSAGE);
        UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, triggerEvent, &value, fieldMessage);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Message failed", UA_NodeId_deleteMembers(&triggerEvent););

        /* Set Comment. Check whether comment is empty -> leave the last value as is*/
        UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
        UA_String nullString = UA_STRING_NULL;
        if(!UA_ByteString_equal(&inputComment->locale, &nullString) &&
           !UA_ByteString_equal(&inputComment->text, &nullString)) {
            UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, triggerEvent, &value, fieldComment);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Comment failed", UA_NodeId_deleteMembers(&triggerEvent););
        }

        /* Get conditionSource */
        UA_NodeId conditionSource;
        retval = getNodeIdValueOfConditionField(server, &triggerEvent, UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENODE), &conditionSource);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionSource not found", UA_NodeId_deleteMembers(&triggerEvent););

        /* Trigger event */
        retval = UA_Server_triggerConditionEvent(server, triggerEvent, conditionSource,
                                                 NULL); //Condition Nodes should not be deleted after triggering the event
        UA_NodeId_deleteMembers(&conditionSource);
        UA_NodeId_deleteMembers(&triggerEvent);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);
    }
    else
        return UA_STATUSCODE_BADCONDITIONDISABLED;

    return retval;
}

static UA_StatusCode
acknowledgeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output) {
    UA_QualifiedName fieldAckedState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ACKEDSTATE);
    UA_QualifiedName fieldAckedStateId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_QualifiedName fieldComment = UA_QUALIFIEDNAME(0, CONDITION_FIELD_COMMENT);
    UA_NodeId conditionNode;
    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event */
    UA_StatusCode retval = getConditionBranchNodeId(server, (UA_ByteString *)input[0].data, &conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionId based on EventId not found",);

    /* Check if retained */
    if(isRetained(server, &conditionNode)) {
        /* Check if already acknowledged */
        if(!isTwoStateVariableInTrueState(server, &conditionNode, &fieldAckedState)) {
            /* Get EventType */
            UA_NodeId eventType;
            retval = getNodeIdValueOfConditionField(server, &conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_EVENTTYPE), &eventType);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "EventType not found", UA_NodeId_deleteMembers(&conditionNode););

            /* Check if ConditionType is subType of AcknowledgeableConditionType TODO Over Kill*/
            UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
            UA_NodeId AcknowledgeableConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
            if(!isNodeInTree(server, &eventType, &AcknowledgeableConditionTypeId, &hasSubtypeId, 1)) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                             "Condition Type must be a subtype of AcknowledgeableConditionType!");
                UA_NodeId_deleteMembers(&conditionNode);
                UA_NodeId_deleteMembers(&eventType);
                return UA_STATUSCODE_BADNODEIDINVALID;
            }

            UA_NodeId_deleteMembers(&eventType);

            /* Set Comment. Check whether comment is empty -> leave the last value as is*/
            UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
            UA_String nullString = UA_STRING_NULL;
            if(!UA_ByteString_equal(&inputComment->locale, &nullString) &&
               !UA_ByteString_equal(&inputComment->text, &nullString)) {
                UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
                retval = UA_Server_setConditionField(server, conditionNode, &value, fieldComment);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Comment failed", UA_NodeId_deleteMembers(&conditionNode););
            }
            /* Set AcknowledgeableStateId */
            UA_Boolean idValue = true;
            UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value, fieldAckedState, fieldAckedStateId);
            UA_NodeId_deleteMembers(&conditionNode);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Acknowledge Condition failed",);
        }
        else
            return UA_STATUSCODE_BADCONDITIONBRANCHALREADYACKED;
    }
    else
        return UA_STATUSCODE_BADCONDITIONDISABLED;

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
    UA_QualifiedName fieldConfirmedState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_CONFIRMEDSTATE);
    UA_QualifiedName fieldConfirmedStateId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);
    UA_QualifiedName fieldComment = UA_QUALIFIEDNAME(0, CONDITION_FIELD_COMMENT);
    UA_NodeId conditionNode;
    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event */
    UA_StatusCode retval = getConditionBranchNodeId(server, (UA_ByteString *)input[0].data, &conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionId based on EventId not found",);

    /* Check if retained */
    if(isRetained(server, &conditionNode)) {
        /* Check if already confirmed */
        if(!isTwoStateVariableInTrueState(server, &conditionNode, &fieldConfirmedState)) {
            /* Get EventType */
            UA_NodeId eventType;
            retval = getNodeIdValueOfConditionField(server, &conditionNode, UA_QUALIFIEDNAME(0, CONDITION_FIELD_EVENTTYPE), &eventType);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "EventType not found", UA_NodeId_deleteMembers(&conditionNode););

            /* Check if ConditionType is subType of AcknowledgeableConditionType. */
            UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
            UA_NodeId AcknowledgeableConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
            if(!isNodeInTree(server, &eventType, &AcknowledgeableConditionTypeId, &hasSubtypeId, 1)) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                             "Condition Type must be a subtype of AcknowledgeableConditionType!");
                UA_NodeId_deleteMembers(&conditionNode);
                UA_NodeId_deleteMembers(&eventType);
                return UA_STATUSCODE_BADNODEIDINVALID;
            }

            UA_NodeId_deleteMembers(&eventType);

            /* Set Comment. Check whether comment is empty -> leave the last value as is*/
            UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
            UA_String nullString = UA_STRING_NULL;
            if(!UA_ByteString_equal(&inputComment->locale, &nullString) &&
               !UA_ByteString_equal(&inputComment->text, &nullString)) {
                UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
                retval = UA_Server_setConditionField(server, conditionNode, &value, fieldComment);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Comment failed", UA_NodeId_deleteMembers(&conditionNode););
            }

            /* Set ConfirmedStateId */
            UA_Boolean idValue = true;
            UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value, fieldConfirmedState, fieldConfirmedStateId);
            UA_NodeId_deleteMembers(&conditionNode);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Acknowledge Condition failed",);
        }
        else
            return UA_STATUSCODE_BADCONDITIONBRANCHALREADYCONFIRMED;
    }
    else
        return UA_STATUSCODE_BADCONDITIONDISABLED;

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
    UA_StatusCode retval = UA_Server_setConditionField(server, *refreshEventNodId, &value, fieldSeverity);
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

    UA_ByteString_deleteMembers(&eventId);

    return retval;
}

static UA_StatusCode
createRefreshMethodEvents(UA_Server *server, UA_NodeId *outRefreshStartNodId, UA_NodeId *outRefreshEndNodId) {
    UA_NodeId refreshStartEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHSTARTEVENTTYPE);
    UA_NodeId refreshEndEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHENDEVENTTYPE);
    /* create RefreshStartEvent TODO those ReferenceTypes should not be Abstract */
    UA_StatusCode retval = UA_Server_createEvent(server, refreshStartEventTypeNodeId, outRefreshStartNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "CreateEvent RefreshStart failed",);

    /* Set Standard Fields */
    retval = setRefreshMethodEventFields(server, outRefreshStartNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Fields of RefreshStartEvent failed",);

    /* create RefreshEndEvent */
    retval = UA_Server_createEvent(server, refreshEndEventTypeNodeId, outRefreshEndNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "CreateEvent RefreshEnd failed",);

    /* Set Standard Fields */
    retval = setRefreshMethodEventFields(server, outRefreshEndNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Fields of RefreshEndEvent failed",);

    return retval;
}

static UA_Boolean
isConditionSourceInMonitoredItem(UA_Server *server, const UA_MonitoredItem *monitoredItem, const UA_NodeId *conditionSource){
    /* TODO: check also other hierarchical references */
    UA_NodeId parentReferences_conditions[4] =
    {
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASEVENTSOURCE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASNOTIFIER}}
    };

    UA_Boolean isConditionSourceInMonItem = isNodeInTree(server, conditionSource,
                                                         &monitoredItem->monitoredNodeId,
                                                         parentReferences_conditions, (sizeof(parentReferences_conditions)/sizeof(parentReferences_conditions[0])));

    return isConditionSourceInMonItem;
}

static UA_StatusCode
refreshLogic(UA_Server *server, const UA_NodeId *refreshStartNodId,
             const UA_NodeId *refreshEndNodId, UA_MonitoredItem *monitoredItem) {
    if(monitoredItem == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* 1. Trigger RefreshStartEvent */
    UA_DateTime fieldTimeValue = UA_DateTime_now();
    UA_StatusCode retval = UA_Server_writeObjectProperty_scalar(server, *refreshStartNodId,
                                                                UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
                                                                &fieldTimeValue, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Write Object Property scalar failed",);

    retval = UA_Event_addEventToMonitoredItem(server, refreshStartNodId, monitoredItem);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Events: Could not add the event to a listening node",);

    /* 2. refresh (see 5.5.7)*/
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        UA_NodeId conditionSource = conditionSourceEntryTmp->conditionSourceId;
        UA_NodeId serverObjectNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
        /* Check if the conditionSource is being monitored. If the Server Object is being monitored,
         * then all Events of all monitoredItems should be refreshed*/
        if(UA_NodeId_equal(&monitoredItem->monitoredNodeId, &conditionSource) ||
           UA_NodeId_equal(&monitoredItem->monitoredNodeId, &serverObjectNodeId) ||
           isConditionSourceInMonitoredItem(server, monitoredItem, &conditionSource)) {
            /* Get Condition Entry */
            UA_Condition_nodeListElement *conditionEntryTmp;
            LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
                /* Get Branch Entry*/
                UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
                LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                    /*if no event was triggered for that branch, then check next without refreshing*/
                    if(UA_ByteString_equal(&conditionBranchEntryTmp->lastEventId, &UA_BYTESTRING_NULL))
                        continue;

                    UA_NodeId triggeredNode;
                    if(conditionBranchEntryTmp->conditionBranchId == NULL)
                        triggeredNode = conditionEntryTmp->conditionId;
                    else
                        triggeredNode = *conditionBranchEntryTmp->conditionBranchId;

                    /* Check if Retain is set to true*/
                    if(isRetained(server, &triggeredNode)) {
                        retval = UA_Event_addEventToMonitoredItem(server, &triggeredNode, monitoredItem);
                        CONDITION_ASSERT_RETURN_RETVAL(retval, "Events: Could not add the event to a listening node",);
                    }
                }
            }
        }
    }

    /* 3. Trigger RefreshEndEvent*/
    fieldTimeValue = UA_DateTime_now();
    retval = UA_Server_writeObjectProperty_scalar(server, *refreshEndNodId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_TIME),
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
    UA_Session *session = UA_SessionManager_getSessionById(&server->sessionManager, sessionId);
    UA_Subscription *subscription = UA_Session_getSubscriptionById(session, *((UA_UInt32 *)input[0].data));
    if(subscription == NULL)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    else {
        /* create RefreshStartEvent and RefreshEndEvent */
        UA_NodeId refreshStartNodId;
        UA_NodeId refreshEndNodId;
        UA_StatusCode retval = createRefreshMethodEvents(server, &refreshStartNodId, &refreshEndNodId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Create Event RefreshStart or RefreshEnd failed",);

        /* Trigger RefreshStartEvent and RefreshEndEvent for the each monitoredItem in the subscription */
        UA_MonitoredItem *monitoredItem = UA_Subscription_getMonitoredItem(subscription, *((UA_UInt32 *)input[1].data));
        if(monitoredItem == NULL)
            return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        else {//TODO when there are a lot of monitoreditems (not only events)?
            retval = refreshLogic(server, &refreshStartNodId, &refreshEndNodId, monitoredItem);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Could not refresh Condition",);
        }

        /* delete RefreshStartEvent and RefreshEndEvent */
        retval = UA_Server_deleteNode(server, refreshStartNodId, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Attempt to remove event using deleteNode failed",);

        retval = UA_Server_deleteNode(server, refreshEndNodId, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Attempt to remove event using deleteNode failed",);
    }

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
    UA_Session *session = UA_SessionManager_getSessionById(&server->sessionManager, sessionId);
    UA_Subscription *subscription = UA_Session_getSubscriptionById(session, *((UA_UInt32 *)input[0].data));
    if(subscription == NULL)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    else {
        /* create RefreshStartEvent and RefreshEndEvent */
        UA_NodeId refreshStartNodId;
        UA_NodeId refreshEndNodId;
        UA_StatusCode retval = createRefreshMethodEvents(server, &refreshStartNodId, &refreshEndNodId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Create Event RefreshStart or RefreshEnd failed",);

        /* Trigger RefreshStartEvent and RefreshEndEvent for the each monitoredItem in the subscription */
        UA_MonitoredItem *monitoredItem = NULL;
        LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {//TODO when there are a lot of monitoreditems (not only events)?
            retval = refreshLogic(server, &refreshStartNodId, &refreshEndNodId, monitoredItem);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Could not refresh Condition",);
        }

        /* delete RefreshStartEvent and RefreshEndEvent */
        retval = UA_Server_deleteNode(server, refreshStartNodId, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Attempt to remove event using deleteNode failed",);

        retval = UA_Server_deleteNode(server, refreshEndNodId, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Attempt to remove event using deleteNode failed",);
    }

    return UA_STATUSCODE_GOOD;
}

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

static UA_StatusCode
setConditionInConditionList(UA_Server *server,
                            const UA_NodeId *conditionNodeId,
                            UA_ConditionSource_nodeListElement *conditionSourceEntry) {
    UA_Condition_nodeListElement *conditionListEntry = (UA_Condition_nodeListElement*) UA_malloc(sizeof(UA_Condition_nodeListElement));
    if(!conditionListEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memset(conditionListEntry, 0, sizeof(UA_Condition_nodeListElement));

    /* Set ConditionId with given ConditionNodeId */
    UA_StatusCode retval = UA_NodeId_copy(conditionNodeId, &conditionListEntry->conditionId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(conditionListEntry);
        return retval;
    }

    UA_ConditionBranch_nodeListElement *conditionBranchListEntry;
    conditionBranchListEntry = (UA_ConditionBranch_nodeListElement*) UA_malloc(sizeof(UA_ConditionBranch_nodeListElement));
    if(!conditionBranchListEntry) {
		UA_free(conditionListEntry);
        return UA_STATUSCODE_BADOUTOFMEMORY;
	}

    memset(conditionBranchListEntry, 0, sizeof(UA_ConditionBranch_nodeListElement));

    /* append to list */
    LIST_INSERT_HEAD(&conditionSourceEntry->conditionHead, conditionListEntry, listEntry);
    LIST_INSERT_HEAD(&conditionListEntry->conditionBranchHead, conditionBranchListEntry, listEntry);

    return retval;
}

static UA_StatusCode
appendConditionEntry(UA_Server *server,
                     const UA_NodeId *conditionNodeId,
                     const UA_NodeId *conditionSourceNodeId) {
    /* Get ConditionSource Entry to see if the ConditionSource Entry already exists*/
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    if(!LIST_EMPTY(&server->headConditionSource)) {
        LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
            if(UA_NodeId_equal(&conditionSourceEntryTmp->conditionSourceId, conditionSourceNodeId)) {
                return setConditionInConditionList(server, conditionNodeId, conditionSourceEntryTmp);
            }
        }
    }
    /* ConditionSource not found in list, so we create a new ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceListEntry;
    conditionSourceListEntry = (UA_ConditionSource_nodeListElement*) UA_malloc(sizeof(UA_ConditionSource_nodeListElement));
    if(!conditionSourceListEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memset(conditionSourceListEntry, 0, sizeof(UA_ConditionSource_nodeListElement));

    /* Set ConditionSourceId with given ConditionSourceNodeId */
    UA_StatusCode retval = UA_NodeId_copy(conditionSourceNodeId, &conditionSourceListEntry->conditionSourceId);
    if(retval != UA_STATUSCODE_GOOD) {
      UA_free(conditionSourceListEntry);
      return retval;
    }
    /* append to list */
    LIST_INSERT_HEAD(&server->headConditionSource, conditionSourceListEntry, listEntry);

    retval = setConditionInConditionList(server, conditionNodeId, conditionSourceListEntry);

    return retval;
}

void
UA_ConditionList_delete(UA_Server *server) {
    UA_ConditionSource_nodeListElement *conditionSourceEntry, *conditionSourceEntryTmp;
    LIST_FOREACH_SAFE(conditionSourceEntry, &server->headConditionSource, listEntry, conditionSourceEntryTmp) {
        UA_Condition_nodeListElement *conditionEntry, *conditionEntryTmp;
        LIST_FOREACH_SAFE(conditionEntry, &conditionSourceEntry->conditionHead, listEntry, conditionEntryTmp) {
            UA_ConditionBranch_nodeListElement *conditionBranchEntry, *conditionBranchEntryTmp;
            LIST_FOREACH_SAFE(conditionBranchEntry, &conditionEntry->conditionBranchHead, listEntry, conditionBranchEntryTmp) {
                if(conditionBranchEntry->conditionBranchId != NULL)
                    UA_NodeId_delete(conditionBranchEntry->conditionBranchId);

                UA_ByteString_deleteMembers(&conditionBranchEntry->lastEventId);
                LIST_REMOVE(conditionBranchEntry, listEntry);
                UA_free(conditionBranchEntry);
            }

            UA_NodeId_deleteMembers(&conditionEntry->conditionId);
            LIST_REMOVE(conditionEntry, listEntry);
            UA_free(conditionEntry);
        }

        UA_NodeId_deleteMembers(&conditionSourceEntry->conditionSourceId);
        LIST_REMOVE(conditionSourceEntry, listEntry);
        UA_free(conditionSourceEntry);
    }
}

/*
 * this function is used to get the ConditionId based on the EventId (all branches of one condition
 * should have the same ConditionId)
 */
UA_StatusCode
UA_getConditionId(UA_Server *server,
                  const UA_NodeId *conditionNodeId,
                  UA_NodeId *outConditionId) {
    /* Get ConditionSource Entry */
    UA_ConditionSource_nodeListElement *conditionSourceEntryTmp;
    LIST_FOREACH(conditionSourceEntryTmp, &server->headConditionSource, listEntry) {
        /* Get Condition Entry */
        UA_Condition_nodeListElement *conditionEntryTmp;
        LIST_FOREACH(conditionEntryTmp, &conditionSourceEntryTmp->conditionHead, listEntry) {
            if(UA_NodeId_equal(&conditionEntryTmp->conditionId, conditionNodeId)) {
                *outConditionId = conditionEntryTmp->conditionId;
                return UA_STATUSCODE_GOOD;
            }
            else {
                /* Get Branch Entry*/
                UA_ConditionBranch_nodeListElement *conditionBranchEntryTmp;
                LIST_FOREACH(conditionBranchEntryTmp, &conditionEntryTmp->conditionBranchHead, listEntry) {
                    if(((NULL == conditionBranchEntryTmp->conditionBranchId) && (NULL == conditionNodeId)) ||
                       (NULL != conditionBranchEntryTmp->conditionBranchId &&
                        UA_NodeId_equal(conditionBranchEntryTmp->conditionBranchId, conditionNodeId))) {
                        *outConditionId = conditionEntryTmp->conditionId;
                        return UA_STATUSCODE_GOOD;
                    }
                }
            }
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

/**
 * function to check whether the Condition Source Node has "EventSource" or
 * one of its subtypes inverse reference.
 */
static UA_Boolean
doesHasEventSourceReferenceExist(UA_Server *server, const UA_NodeId nodeToCheck)
{
    const UA_Node* node = UA_NODESTORE_GET(server, &nodeToCheck);
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_NodeId hasEventSourceId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE);
    if(node != NULL) {
        for(size_t i = 0; i < node->referencesSize; i++) {
            if((UA_NodeId_equal(&node->references[i].referenceTypeId, &hasEventSourceId) ||
               isNodeInTree(server, &node->references[i].referenceTypeId, &hasEventSourceId, &hasSubtypeId, 1)) &&
               (node->references[i].isInverse == true)) {
                UA_NODESTORE_RELEASE(server, node);
                return true;
            }
        }
    }

    UA_NODESTORE_RELEASE(server, node);
    return false;
}

static UA_StatusCode
setStandardConditionFields(UA_Server *server,
                           const UA_NodeId* condition,
                           const UA_NodeId* conditionType,
                           const UA_NodeId* conditionSource,
                           UA_QualifiedName* conditionName) {
    UA_Variant value;
    /* Set Fields */
    /* 1.Set EventType */
    UA_Variant_setScalar(&value, (void*)(uintptr_t)conditionType, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_EVENTTYPE));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EventType Field failed",);

    /* 2.Set ConditionName */
    UA_Variant_setScalar(&value, &conditionName->name, &UA_TYPES[UA_TYPES_STRING]);
    retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_CONDITIONNAME));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ConditionName Field failed",);

    /* 3.Set EnabledState (Disabled by default -> Retain Field = false) */
    UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, DISABLED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_ENABLEDSTATE));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState Field failed",);

    /* 4.Set EnabledState/Id */
    UA_Boolean stateId = false;
    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(server, *condition, &value,
                                               UA_QUALIFIEDNAME(0,CONDITION_FIELD_ENABLEDSTATE),
                                               UA_QUALIFIEDNAME(0,CONDITION_FIELD_TWOSTATEVARIABLE_ID));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState/Id Field failed",);

    /* 5.Set Retain*/
    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_RETAIN));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Retain Field failed",);

    /* Get ConditionSourceNode*/
    const UA_Node *conditionSourceNode = UA_NODESTORE_GET(server, conditionSource);
    if(NULL == conditionSourceNode) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Couldn't find ConditionSourceNode. StatusCode %s", UA_StatusCode_name(retval));
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /*6.Set SourceName*/
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&conditionSourceNode->browseName.name, &UA_TYPES[UA_TYPES_STRING]);
    retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_SOURCENAME));
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Set SourceName Field failed. StatusCode %s", UA_StatusCode_name(retval));
        UA_NODESTORE_RELEASE(server, conditionSourceNode);
        return retval;
    }

    /*7.Set SourceNode*/
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&conditionSourceNode->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_SOURCENODE));
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Set SourceNode Field failed. StatusCode %s", UA_StatusCode_name(retval));
        UA_NODESTORE_RELEASE(server, conditionSourceNode);
        return retval;
    }

    UA_NODESTORE_RELEASE(server, conditionSourceNode);

    /*8.Set Quality (TODO not supported, thus set with Status Good)*/
    UA_StatusCode qualityValue = UA_STATUSCODE_GOOD;
    UA_Variant_setScalar(&value, &qualityValue, &UA_TYPES[UA_TYPES_STATUSCODE]);
    retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_QUALITY));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Quality Field failed",);

    /*9.Set Severity*/
    UA_UInt16 severityValue = 0;
    UA_Variant_setScalar(&value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_SEVERITY));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Severity Field failed",);

    /* Check subTypes of ConditionType to set further Fields*/

    /* 1. Check if ConditionType is subType of AcknowledgeableConditionType */
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_NodeId acknowledgeableConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
    if(isNodeInTree(server, conditionType, &acknowledgeableConditionTypeId, &hasSubtypeId, 1)) {
        /* Set AckedState (Id = false by default) */
        text = UA_LOCALIZEDTEXT(LOCALE, UNACKED_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_ACKEDSTATE));
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set AckedState Field failed",);

        UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval = UA_Server_setConditionVariableFieldProperty(server, *condition, &value,
                                                   UA_QUALIFIEDNAME(0,CONDITION_FIELD_ACKEDSTATE),
                                                   UA_QUALIFIEDNAME(0,CONDITION_FIELD_TWOSTATEVARIABLE_ID));
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState/Id Field failed",);

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
        /* add optional field ConfirmedState*/
        retval = UA_Server_addConditionOptionalField(server, *condition, acknowledgeableConditionTypeId,
                                                     UA_QUALIFIEDNAME(0,CONDITION_FIELD_CONFIRMEDSTATE), NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding ConfirmedState optional Field failed",);

        /* Set ConfirmedState (Id = false by default) */
        text = UA_LOCALIZEDTEXT(LOCALE, UNCONFIRMED_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_CONFIRMEDSTATE));
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ConfirmedState Field failed",);

        UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval = UA_Server_setConditionVariableFieldProperty(server, *condition, &value,
                                                   UA_QUALIFIEDNAME(0,CONDITION_FIELD_CONFIRMEDSTATE),
                                                   UA_QUALIFIEDNAME(0,CONDITION_FIELD_TWOSTATEVARIABLE_ID));
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState/Id Field failed",);
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

        /* 2. Check if ConditionType is subType of AlarmConditionType */
        UA_NodeId alarmConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE);
        if(isNodeInTree(server, conditionType, &alarmConditionTypeId, &hasSubtypeId, 1)) {
            /* Set ActiveState (Id = false by default) */
            text = UA_LOCALIZEDTEXT(LOCALE, INACTIVE_TEXT);
            UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0,CONDITION_FIELD_ACTIVESTATE));
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ActiveState Field failed",);
        }
    }

    return retval;
}

/**
 * Set callbacks for TwoStateVariable Fields of a condition
 */
static UA_StatusCode
setTwoStateVariableCallbacks(UA_Server *server,
                             const UA_NodeId* condition,
                             const UA_NodeId* conditionType) {
    UA_QualifiedName fieldEnabledState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    UA_QualifiedName fieldAckedState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ACKEDSTATE);
    UA_QualifiedName fieldConfirmedState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_CONFIRMEDSTATE);
    UA_QualifiedName fieldActiveState = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ACTIVESTATE);
    UA_QualifiedName twoStateVariableId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_TWOSTATEVARIABLE_ID);

    /* Set EnabledState Callback */
    UA_NodeId twoStateVariableIdNodeId = UA_NODEID_NULL;
    UA_StatusCode retval = getConditionFieldPropertyNodeId(server, condition, &fieldEnabledState, &twoStateVariableId, &twoStateVariableIdNodeId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

    UA_ValueCallback callback;
    callback.onRead = NULL;
    callback.onWrite = afterWriteCallbackEnabledStateChange;
    retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId, callback);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState Callback failed", UA_NodeId_deleteMembers(&twoStateVariableIdNodeId););

    /* Set AckedState Callback */
    /* Check if ConditionType is subType of AcknowledgeableConditionType */
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_NodeId acknowledgeableConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
    if(isNodeInTree(server, conditionType, &acknowledgeableConditionTypeId, &hasSubtypeId, 1)) {
        UA_NodeId_deleteMembers(&twoStateVariableIdNodeId);
        retval = getConditionFieldPropertyNodeId(server, condition, &fieldAckedState, &twoStateVariableId, &twoStateVariableIdNodeId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

        callback.onWrite = afterWriteCallbackAckedStateChange;
        retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId, callback);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set AckedState Callback failed", UA_NodeId_deleteMembers(&twoStateVariableIdNodeId););

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
        /* add callback */
        callback.onWrite = afterWriteCallbackConfirmedStateChange;
        UA_NodeId_deleteMembers(&twoStateVariableIdNodeId);
        retval = getConditionFieldPropertyNodeId(server, condition, &fieldConfirmedState, &twoStateVariableId, &twoStateVariableIdNodeId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

        /* add reference from Condition to Confirm Method */
        retval = UA_Server_addReference(server, *condition, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM), true);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding HasComponent Reference to Confirm Method failed", UA_NodeId_deleteMembers(&twoStateVariableIdNodeId););

        retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId, callback);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding ConfirmedState/Id callback failed", UA_NodeId_deleteMembers(&twoStateVariableIdNodeId););
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

        /* Set ActiveState Callback */
        /* Check if ConditionType is subType of AlarmConditionType */
        UA_NodeId alarmConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE);
        if(isNodeInTree(server, conditionType, &alarmConditionTypeId, &hasSubtypeId, 1)) {
            UA_NodeId_deleteMembers(&twoStateVariableIdNodeId);
            retval = getConditionFieldPropertyNodeId(server, condition, &fieldActiveState, &twoStateVariableId, &twoStateVariableIdNodeId);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

            callback.onWrite = afterWriteCallbackActiveStateChange;
            retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId, callback);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ActiveState Callback failed", UA_NodeId_deleteMembers(&twoStateVariableIdNodeId););
        }
    }

    UA_NodeId_deleteMembers(&twoStateVariableIdNodeId);
    return retval;
}

/**
 * Set callbacks for ConditionVariable Fields of a condition
 */
static UA_StatusCode
setConditionVariableCallbacks(UA_Server *server,
                              const UA_NodeId* condition,
                              const UA_NodeId* conditionType) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_QualifiedName conditionVariableName[2] = {
        UA_QUALIFIEDNAME(0, CONDITION_FIELD_QUALITY),
        UA_QUALIFIEDNAME(0, CONDITION_FIELD_SEVERITY)
    };// extend array with other fields when needed

    for(size_t i=0; (i<sizeof(conditionVariableName)/sizeof(conditionVariableName[0])) && (retval == UA_STATUSCODE_GOOD); i++) {
        UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *condition, 1, &conditionVariableName[i]);
        if(bpr.statusCode != UA_STATUSCODE_GOOD) {
            return bpr.statusCode;
        }
        UA_ValueCallback callback ;
        callback.onRead = NULL;
        switch(i) {
            case 0:
                callback.onWrite = afterWriteCallbackQualityChange;
                break;
            case 1:
                callback.onWrite = afterWriteCallbackSeverityChange;
                break;
                // add other callbacks here
            default:
                UA_BrowsePathResult_deleteMembers(&bpr);
                return UA_STATUSCODE_BADNOTSUPPORTED;
        }

        retval = UA_Server_setVariableNode_valueCallback(server, bpr.targets[0].targetId.nodeId, callback);
        UA_BrowsePathResult_deleteMembers(&bpr);
    }

    return retval;
}

/**
 * Set callbacks for Method Fields of a condition.
 * The current implementation references methods without copying them when
 * creating objects. So the callbacks will be attached to the methods of
 * the conditionType.
 */
static UA_StatusCode
setConditionMethodCallbacks(UA_Server *server,
                            const UA_NodeId* condition,
                            const UA_NodeId* conditionType) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* add callbacks to methods of the Conditiontype */
    UA_NodeId methodId[7] = {
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_DISABLE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_ENABLE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_ADDCOMMENT}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE}}
#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
        ,{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM}}
#endif//CONDITIONOPTIONALFIELDS_SUPPORT
    };

    for(size_t i=0; (i<(sizeof(methodId)/sizeof(methodId[0]))) && (retval == UA_STATUSCODE_GOOD); i++) {
        UA_MethodCallback methodCallback = NULL;
        switch(i) {
            case 0:
                methodCallback = disableMethodCallback;
                break;
            case 1:
                methodCallback = enableMethodCallback;
                break;
            case 2:
                methodCallback = addCommentMethodCallback;
                break;
            case 3:
                methodCallback = refreshMethodCallback;
                break;
            case 4:
                methodCallback = refresh2MethodCallback;
                break;
            case 5:
                methodCallback = acknowledgeMethodCallback;
                break;
#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
            case 6:
                methodCallback = confirmMethodCallback;
                break;
#endif//CONDITIONOPTIONALFIELDS_SUPPORT
                // add other callbacks here
            default:
                return UA_STATUSCODE_BADNOTSUPPORTED;
        }

        retval = UA_Server_setMethodNode_callback(server, methodId[i], methodCallback);
    }

    return retval;
}

static UA_StatusCode
setStandardConditionCallbacks(UA_Server *server,
                              const UA_NodeId* condition,
                              const UA_NodeId* conditionType) {
    UA_StatusCode retval = setTwoStateVariableCallbacks(server, condition, conditionType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set TwoStateVariable Callback failed",);

    retval = setConditionVariableCallbacks(server, condition, conditionType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ConditionVariable Callback failed",);

    /* Set callbacks for Method Components */
    retval = setConditionMethodCallbacks(server, condition, conditionType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Method Callback failed",);

    return retval;
}

/**
 * create condition instance. The function checks first whether the passed conditionType
 * is a subType of ConditionType. Then checks whether the condition source has HasEventSource
 * reference to its parent. If not, a HasEventSource reference will be created between condition
 * source and server object. To expose the condition in address space, a hierarchical ReferenceType
 * should be passed to create the reference to condition source. Otherwise, UA_NODEID_NULL should be
 * passed to make the condition unexposed.
 */
UA_StatusCode
UA_Server_createCondition(UA_Server *server,
                          const UA_NodeId conditionId, const UA_NodeId conditionType,
                          UA_QualifiedName conditionName, const UA_NodeId conditionSource,
                          const UA_NodeId hierarchialReferenceType, UA_NodeId *outNodeId) {
    UA_StatusCode retval;

    if(!outNodeId) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Make sure the conditionType is a Subtype of ConditionType */
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(!isNodeInTree(server, &conditionType, &conditionTypeId, &hasSubtypeId, 1)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Condition Type must be a subtype of ConditionType!");
        return UA_STATUSCODE_BADNOMATCH;
    }

    /* Make sure the ConditionSource has HasEventSource or one of its SubTypes ReferenceType */
    UA_NodeId serverObject = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    if(!doesHasEventSourceReferenceExist(server, conditionSource) &&
       !UA_NodeId_equal(&serverObject, &conditionSource)) {
         UA_NodeId hasHasEventSourceId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE);

          retval = UA_Server_addReference(server, serverObject, hasHasEventSourceId,
                                          UA_EXPANDEDNODEID_NUMERIC(conditionSource.namespaceIndex,
                                                                    conditionSource.identifier.numeric),
                                          true);
          CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasHasEventSource Reference to the Server Object failed",);
    }

    /* Create an ObjectNode which represents the condition */
    UA_NodeId newNodeId = UA_NODEID_NULL;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en", (char*)conditionName.name.data);
    retval = UA_Server_addObjectNode(server,
                                     conditionId,
                                     UA_NODEID_NULL,
                                     UA_NODEID_NULL,
                                     conditionName,
                                     conditionType,  /* the type of the Condition */
                                     oAttr,          /* default attributes are fine */
                                     NULL,           /* no node context */
                                     &newNodeId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding Condition failed",);

    /* create HasCondition Reference (HasCondition should be forward from the ConditionSourceNode to the Condition.
     * else, HasCondition should be forward from the ConditionSourceNode to the ConditionType Node) */
    UA_NodeId nodIdNull = UA_NODEID_NULL;
    UA_ExpandedNodeId hasConditionTarget;
    if(!UA_NodeId_equal(&hierarchialReferenceType, &nodIdNull)) {
        hasConditionTarget = UA_EXPANDEDNODEID_NUMERIC(newNodeId.namespaceIndex, newNodeId.identifier.numeric);

        /* create hierarchical Reference to ConditionSource to expose the ConditionNode in Address Space */
        retval = UA_Server_addReference(server, conditionSource, hierarchialReferenceType,
                                        UA_EXPANDEDNODEID_NUMERIC(newNodeId.namespaceIndex, newNodeId.identifier.numeric), true);// only Check hierarchialReferenceType
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating hierarchical Reference to ConditionSource failed",);
    }
    else
        hasConditionTarget = UA_EXPANDEDNODEID_NUMERIC(conditionType.namespaceIndex, conditionType.identifier.numeric);

    UA_NodeId hasConditionId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCONDITION);
    retval = UA_Server_addReference(server, conditionSource, hasConditionId,
                                    hasConditionTarget, true);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasCondition Reference failed",);

    /* Set standard fields */
    retval = setStandardConditionFields(server, &newNodeId, &conditionType,
                                        &conditionSource, &conditionName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Condition Fields failed",);

    *outNodeId = newNodeId;

    /* Set Method Callbacks */
    retval = setStandardConditionCallbacks(server, &newNodeId, &conditionType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition callbacks failed",);

    /* change Refresh Events IsAbstract = false
     * so abstract Events : RefreshStart and RefreshEnd could be created
     */
    UA_NodeId refreshStartEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHSTARTEVENTTYPE);
    UA_NodeId refreshEndEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHENDEVENTTYPE);
    const UA_Node* refreshStartEventType = UA_NODESTORE_GET(server, &refreshStartEventTypeNodeId);
    const UA_Node* refreshEndEventType = UA_NODESTORE_GET(server, &refreshEndEventTypeNodeId);

    if(false == ((const UA_ObjectTypeNode*)refreshStartEventType)->isAbstract &&
       false == ((const UA_ObjectTypeNode*)refreshEndEventType)->isAbstract) {
        UA_NODESTORE_RELEASE(server, refreshStartEventType);
        UA_NODESTORE_RELEASE(server, refreshEndEventType);
    }
    else
    {
        UA_NODESTORE_RELEASE(server, refreshStartEventType);
        UA_NODESTORE_RELEASE(server, refreshEndEventType);
        UA_Node* refreshStartEventTypeInner;
        UA_Node* refreshEndEventTypeInner;
        if(UA_STATUSCODE_GOOD != UA_NODESTORE_GETCOPY(server, &refreshStartEventTypeNodeId, &refreshStartEventTypeInner) ||
           UA_STATUSCODE_GOOD != UA_NODESTORE_GETCOPY(server, &refreshEndEventTypeNodeId, &refreshEndEventTypeInner))
            UA_assert(0);
        else
        {
            ((UA_ObjectTypeNode*)refreshStartEventTypeInner)->isAbstract = false;
            ((UA_ObjectTypeNode*)refreshEndEventTypeInner)->isAbstract = false;
            UA_NODESTORE_REPLACE(server, refreshStartEventTypeInner);
            UA_NODESTORE_REPLACE(server, refreshEndEventTypeInner);
        }
    }

    /* append Condition to list */
    return appendConditionEntry(server, &newNodeId, &conditionSource);
}

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT

static UA_StatusCode
addOptionalVariableField(UA_Server *server,
                         const UA_NodeId *originCondition,
                         const UA_QualifiedName* fieldName,
                         const UA_VariableNode *optionalVariableFieldNode,
                         UA_NodeId *outOptionalVariable) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.valueRank = optionalVariableFieldNode->valueRank;
    UA_StatusCode retval = UA_LocalizedText_copy(&optionalVariableFieldNode->displayName, &vAttr.displayName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Copying LocalizedText failed",);

    retval = UA_NodeId_copy(&optionalVariableFieldNode->dataType, &vAttr.dataType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Copying NodeId failed",);

    /* Get typedefintion */
    const UA_Node *type = getNodeType(server, (const UA_Node *)optionalVariableFieldNode);
    if(type != NULL) {
        /* Set referenceType to parent */
        UA_NodeId referenceToParent;
        UA_NodeId propertyTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE);
        if(UA_NodeId_equal(&type->nodeId, &propertyTypeNodeId))
            referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
        else
            referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);

        UA_NodeId optionalVariable = {originCondition->namespaceIndex, UA_NODEIDTYPE_NUMERIC, {0}};
        retval =
        UA_Server_addVariableNode(server,
                                  optionalVariable, /* Set a random unused NodeId with specified Namespace Index*/
                                  *originCondition,
                                  referenceToParent,
                                  *fieldName,
                                  type->nodeId,   /* the type of the Field */
                                  vAttr,
                                  NULL,           /* no node context */
                                  outOptionalVariable);

        UA_NODESTORE_RELEASE(server, type);
        UA_VariableAttributes_deleteMembers(&vAttr);
    }
    else {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                           "Invalid VariableType. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
        UA_VariableAttributes_deleteMembers(&vAttr);
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    return retval;
}

static UA_StatusCode
addOptionalObjectField(UA_Server *server,
                       const UA_NodeId *originCondition,
                       const UA_QualifiedName* fieldName,
                       const UA_ObjectNode *optionalObjectFieldNode,
                       UA_NodeId *outOptionalObject) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_StatusCode retval = UA_LocalizedText_copy(&optionalObjectFieldNode->displayName, &oAttr.displayName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Copying LocalizedText failed",);

    /* Get typedefintion */
    const UA_Node *type = getNodeType(server, (const UA_Node *)optionalObjectFieldNode);
    if(type != NULL) {
        /* Set referenceType to parent */
        UA_NodeId referenceToParent;
        UA_NodeId propertyTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE);
        if(UA_NodeId_equal(&type->nodeId, &propertyTypeNodeId))
            referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
        else
            referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);

        UA_NodeId optionalObject = {originCondition->namespaceIndex, UA_NODEIDTYPE_NUMERIC, {0}};
        retval =
        UA_Server_addObjectNode(server,
                                optionalObject, /* Set a random unused NodeId with specified Namespace Index*/
                                *originCondition,
                                referenceToParent,
                                *fieldName,
                                type->nodeId,   /* the type of the Field */
                                oAttr,
                                NULL,           /* no node context */
                                outOptionalObject);

        UA_NODESTORE_RELEASE(server, type);
        UA_ObjectAttributes_deleteMembers(&oAttr);
    }
    else {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                           "Invalid ObjectType. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
        UA_ObjectAttributes_deleteMembers(&oAttr);
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

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
    /* Get optional Field NodId from ConditionType -> user should give the correct ConditionType or Subtype!!!! */
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, conditionType, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    /* Get Node */
    UA_NodeId optionalFieldNodeId = bpr.targets[0].targetId.nodeId;
    const UA_Node *optionalFieldNode = UA_NODESTORE_GET(server, &optionalFieldNodeId);
    if(NULL == optionalFieldNode) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Couldn't find optional Field Node in ConditionType. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADNOTFOUND));
        UA_BrowsePathResult_deleteMembers(&bpr);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    switch(optionalFieldNode->nodeClass) {
        case UA_NODECLASS_VARIABLE: {
            UA_StatusCode retval = addOptionalVariableField(server, &condition, &fieldName,
                                              (const UA_VariableNode *)optionalFieldNode, outOptionalNode);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                             "Adding Condition Optional Variable Field failed. StatusCode %s", UA_StatusCode_name(retval));
            }
            UA_BrowsePathResult_deleteMembers(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return retval;
        }
        case UA_NODECLASS_OBJECT:{
          UA_StatusCode retval = addOptionalObjectField(server, &condition, &fieldName,
                                            (const UA_ObjectNode *)optionalFieldNode, outOptionalNode);
          if(retval != UA_STATUSCODE_GOOD) {
              UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                           "Adding Condition Optional Object Field failed. StatusCode %s", UA_StatusCode_name(retval));
          }
          UA_BrowsePathResult_deleteMembers(&bpr);
          UA_NODESTORE_RELEASE(server, optionalFieldNode);
          return retval;
        }
        case UA_NODECLASS_METHOD:
            /*TODO method: Check first logic of creating methods at all (should we create a new method or just reference it from the ConditionType?)*/
            UA_BrowsePathResult_deleteMembers(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
        default:
            UA_BrowsePathResult_deleteMembers(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
    }

#else
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                         "Adding Condition Optional Fields disabled. StatusCode %s", UA_StatusCode_name(UA_STATUSCODE_BADNOTSUPPORTED));
    return UA_STATUSCODE_BADNOTSUPPORTED;
#endif//CONDITIONOPTIONALFIELDS_SUPPORT
}

/**
 * Set the value of condition field (only scalar).
 */
UA_StatusCode
UA_Server_setConditionField(UA_Server *server,
                            const UA_NodeId condition,
                            const UA_Variant* value,
                            const UA_QualifiedName fieldName) {

    if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
      //TODO implement logic for array variants!
      CONDITION_ASSERT_RETURN_RETVAL(UA_STATUSCODE_BADNOTIMPLEMENTED, "Set Condition Field with Array value not implemented",);
    }

    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, condition, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    UA_StatusCode retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, *value);
    UA_BrowsePathResult_deleteMembers(&bpr);

    return retval;
}

/**
 * Set the value of property of condition field.
 */
UA_StatusCode
UA_Server_setConditionVariableFieldProperty(UA_Server *server,
                                            const UA_NodeId condition,
                                            const UA_Variant* value,
                                            const UA_QualifiedName variableFieldName,
                                            const UA_QualifiedName variablePropertyName) {

  if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
      //TODO implement logic for array variants!
      CONDITION_ASSERT_RETURN_RETVAL(UA_STATUSCODE_BADNOTIMPLEMENTED, "Set Property of Condition Field with Array value not implemented",);
    }

    /*1) find Variable Field of the Condition*/
    UA_BrowsePathResult bprConditionVariableField = UA_Server_browseSimplifiedBrowsePath(server, condition, 1, &variableFieldName);
    if(bprConditionVariableField.statusCode != UA_STATUSCODE_GOOD)
        return bprConditionVariableField.statusCode;

    /*2) find Property of the Variable Field of the Condition*/
    UA_BrowsePathResult bprVariableFieldProperty = UA_Server_browseSimplifiedBrowsePath(server, bprConditionVariableField.targets->targetId.nodeId, 1, &variablePropertyName);
    if(bprVariableFieldProperty.statusCode != UA_STATUSCODE_GOOD) {
        UA_BrowsePathResult_deleteMembers(&bprConditionVariableField);
        return bprVariableFieldProperty.statusCode;
    }

    UA_StatusCode retval = UA_Server_writeValue(server, bprVariableFieldProperty.targets[0].targetId.nodeId, *value);

    UA_BrowsePathResult_deleteMembers(&bprConditionVariableField);
    UA_BrowsePathResult_deleteMembers(&bprVariableFieldProperty);

    return retval;
}

/**
 * triggers an event only for an enabled condition. The condition list is updated then with the
 * last generated EventId.
 */
UA_StatusCode
UA_Server_triggerConditionEvent(UA_Server *server, const UA_NodeId condition,
                                const UA_NodeId conditionSource, UA_ByteString *outEventId){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    UA_QualifiedName enabledStateField = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);

    /* Check if enabled */
    if(isTwoStateVariableInTrueState(server, &condition, &enabledStateField)) {
        setIsCallerAC(server, &condition, &conditionSource, true);
        /* Trigger the event for Condition*/
        retval = UA_Server_triggerEvent(server, condition, conditionSource,
                                        &eventId, false); //Condition Nodes should not be deleted after triggering the event
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);

        setIsCallerAC(server, &condition, &conditionSource, false);

        /* Update list */
        retval = updateConditionLastEventId(server, &condition, &conditionSource, &eventId);

        if(outEventId)
            *outEventId = eventId;
        else
            UA_ByteString_deleteMembers(&eventId);
    }
    else {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot trigger condition event when "CONDITION_FIELD_ENABLEDSTATE"."CONDITION_FIELD_TWOSTATEVARIABLE_ID" is false. StatusCode %s", UA_StatusCode_name(retval));
    }

    return retval;
}
#endif//UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
