/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Kalycito Infotech Private Limited (Author: Jayanth Velusamy)
 *
 * */

#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/log_stdout.h>

#include "ua_client_internal.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS

const size_t nACSelectClauses = 24;

void handler_events_alarms_condition(UA_Client *client, UA_UInt32 subId, void *subContext,
               UA_UInt32 monId, void *monContext,
               size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\nNotification:");

    if(UA_Variant_hasScalarType(&eventFields[0], &UA_TYPES[UA_TYPES_BYTESTRING])) {
    UA_ByteString eventId = *(UA_ByteString *)eventFields[0].data;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "EventId: " UA_PRITNF_EVENTID_FORMAT , UA_PRITNF_EVENTID_DATA(eventId));
    }

    /* Message */
    if (UA_Variant_hasScalarType(&eventFields[1], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
       	UA_LocalizedText *lt = (UA_LocalizedText *)eventFields[1].data;
        if(lt) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Message: '%.*s'", (int)lt->text.length, lt->text.data);
        }
    }

    /* Time */
    if(UA_Variant_hasScalarType(&eventFields[2], &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime *time = (UA_DateTime *)eventFields[2].data;
        if(time) {
            UA_DateTimeStruct dts = UA_DateTime_toStruct(*time);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                     dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
        }
    }

    /* SourceName */
    if(UA_Variant_hasScalarType(&eventFields[3], &UA_TYPES[UA_TYPES_STRING])) {
        UA_String *sourceName = (UA_String *)eventFields[3].data;
        if(sourceName) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "SourceName: '%.*s'", (int)sourceName->length, sourceName->data);
        }
    }

    /* Severity */
    if(UA_Variant_hasScalarType(&eventFields[4], &UA_TYPES[UA_TYPES_UINT16])) {
        UA_UInt16 *severity = (UA_UInt16 *)eventFields[4].data;
        if(severity) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Severity: %u", *severity);
        }
    }

    /* EventType */
    if(UA_Variant_hasScalarType(&eventFields[5], &UA_TYPES[UA_TYPES_NODEID])) {
        UA_NodeId *eventType = (UA_NodeId *)eventFields[5].data;
        if(eventType) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "EventType: %u", eventType->identifier.numeric);
        }
    }

    /* EnabledState */
    if(UA_Variant_hasScalarType(&eventFields[6], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
        UA_LocalizedText *enabledState = (UA_LocalizedText *)eventFields[6].data;
        if(enabledState) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "EnabledState: '%.*s'", (int)enabledState->text.length, enabledState->text.data);
        }
    }

    /* LastSeverity */
    if(UA_Variant_hasScalarType(&eventFields[7], &UA_TYPES[UA_TYPES_UINT16])) {
        UA_UInt16 *lastSeverity = (UA_UInt16 *)eventFields[7].data;
        if(lastSeverity) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "LastSeverity: %u", *lastSeverity);
        }
    }

    /* SourceTimeStamp */
    if(UA_Variant_hasScalarType(&eventFields[8], &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime *lastSeveritySourceTimestamp = (UA_DateTime *)eventFields[8].data;
        if(lastSeveritySourceTimestamp) {
            UA_DateTimeStruct dts = UA_DateTime_toStruct(*lastSeveritySourceTimestamp);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                     dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
        }
    }

    /* BranchId */
    //TODO

    /* ConditionName */
    if(UA_Variant_hasScalarType(&eventFields[10], &UA_TYPES[UA_TYPES_STRING])) {
        UA_String *conditionName = (UA_String *)eventFields[10].data;
        if(conditionName) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "conditionName: '%.*s'", (int)conditionName->length, conditionName->data);
        }
    }

    /* Retain */
    if (UA_Variant_hasScalarType(&eventFields[11], &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_Boolean *retain = (UA_Boolean *)eventFields[11].data;
        if(retain) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Retain: '%d'", *retain);
        }
    }

    /* ConditionClassId */
    // TODO

    /* ConditionClassName */
    // TODO

    /* ConfirmedStateId */
    if(UA_Variant_hasScalarType(&eventFields[14], &UA_TYPES[UA_TYPES_BOOLEAN])) {
    UA_Boolean *confirmedStateId = (UA_Boolean *)eventFields[14].data;
        if(confirmedStateId) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ConfirmedStateId: %u", *confirmedStateId);
        }
    }

    /* AckedState/Id */
    if(UA_Variant_hasScalarType(&eventFields[15], &UA_TYPES[UA_TYPES_BOOLEAN])) {
    UA_Boolean *ackedStateId = (UA_Boolean *)eventFields[15].data;
        if(ackedStateId) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AckedState/Id: %d", *ackedStateId);
        }
    }

    /* ActiveState */
    if(UA_Variant_hasScalarType(&eventFields[16], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
    UA_LocalizedText *activeState = (UA_LocalizedText *)eventFields[16].data;
        if(activeState) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "activeState: '%.*s'", (int)activeState->text.length, activeState->text.data);
        }
    }

    /* ActiveState/Id */
    if(UA_Variant_hasScalarType(&eventFields[17], &UA_TYPES[UA_TYPES_BOOLEAN])) {
    UA_Boolean *activeStateId = (UA_Boolean *)eventFields[17].data;
        if(activeStateId) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ActiveState/Id: %d", *activeStateId);
        }
    }

    /* ActiveState/EffectiveDisplayName */
    // TODO

    /* Prompt */
    // TODO

    /* ResponseOptionSet */
    // TODO

    /* DefaultResponse */
    // TODO

    /* DialogState/Id */
    // TODO

    /* ConditionType */
    if(UA_Variant_hasScalarType(&eventFields[23], &UA_TYPES[UA_TYPES_NODEID])) {
    UA_NodeId *conditionType = (UA_NodeId *)eventFields[23].data;
        if(conditionType) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "ConditionType: %u", conditionType->identifier.numeric);
        }
    }
}

UA_SimpleAttributeOperand *
setupSelectClausesAlarmCondition(void) {
    UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(nACSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return NULL;

    for(size_t i =0; i<nACSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
    }

    for(size_t i = 0; i < nACSelectClauses - 1; i++) {
        selectClauses[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        selectClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }

    /* EventId */
    selectClauses[0].browsePathSize = 1;
    selectClauses[0].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[0].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EventId");

    /* Message */
    selectClauses[1].browsePathSize = 1;
    selectClauses[1].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[1].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[1].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");

    /* Time */
    selectClauses[2].browsePathSize = 1;
    selectClauses[2].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[2].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[2].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Time");

    /* SourceName */
    selectClauses[3].browsePathSize = 1;
    selectClauses[3].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[3].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[3].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[3].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "SourceName");

    /* Severity */
    selectClauses[4].browsePathSize = 1;
    selectClauses[4].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[4].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[4].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[4].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");

    /* EventType */
    selectClauses[5].browsePathSize = 1;
    selectClauses[5].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[5].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[5].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[5].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EventType");

    /* EnabledState */
    selectClauses[6].browsePathSize = 1;
    selectClauses[6].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[6].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[6].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[6].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EnabledState");

    /* LastSeverity */
    selectClauses[7].browsePathSize = 1;
    selectClauses[7].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[7].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[7].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[7].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "LastSeverity");

    selectClauses[8].browsePathSize = 2;
    selectClauses[8].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[8].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[8].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[8].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "LastSeverity");
    selectClauses[8].browsePath[1] = UA_QUALIFIEDNAME_ALLOC(0, "SourceTimestamp");

    /* EventId */
    selectClauses[9].browsePathSize = 1;
    selectClauses[9].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[9].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[9].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[9].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "BranchId");

    /* EventId */
    selectClauses[10].browsePathSize = 1;
    selectClauses[10].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[10].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[10].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[10].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ConditionName");

    /* Retain */
    selectClauses[11].browsePathSize = 1;
    selectClauses[11].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[11].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[11].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[11].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Retain");

    /* ConditionClassId */
    selectClauses[12].browsePathSize = 1;
    selectClauses[12].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[12].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[12].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[12].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ConditionClassId");

    /* ConditionClassName */
    selectClauses[13].browsePathSize = 1;
    selectClauses[13].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[13].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[13].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[13].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ConditionClassName");

    /* ConfirmedState/Id */
    selectClauses[14].browsePathSize = 2;
    selectClauses[14].browsePath = (UA_QualifiedName*)
    UA_Array_new(selectClauses[14].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[14].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[14].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ConfirmedState");
    selectClauses[14].browsePath[1] = UA_QUALIFIEDNAME_ALLOC(0, "Id");

    /* AckedState/Id */
    selectClauses[15].browsePathSize = 2;
    selectClauses[15].browsePath = (UA_QualifiedName*)
    UA_Array_new(selectClauses[15].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[15].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[15].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "AckedState");
    selectClauses[15].browsePath[1] = UA_QUALIFIEDNAME_ALLOC(0, "Id");

    /* ActiveState */
    selectClauses[16].browsePathSize = 1;
    selectClauses[16].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[16].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[16].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[16].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ActiveState");

    /* ActiveState/Id */
    selectClauses[17].browsePathSize = 2;
    selectClauses[17].browsePath = (UA_QualifiedName*)
    UA_Array_new(selectClauses[17].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[17].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[17].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ActiveState");
    selectClauses[17].browsePath[1] = UA_QUALIFIEDNAME_ALLOC(0, "Id");

    /* ActiveState/EffectiveDisplayName */
    selectClauses[18].browsePathSize = 2;
    selectClauses[18].browsePath = (UA_QualifiedName*)
    UA_Array_new(selectClauses[18].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[18].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[18].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ActiveState");
    selectClauses[18].browsePath[1] = UA_QUALIFIEDNAME_ALLOC(0, "EffectiveDisplayName");

    /* Prompt */
    selectClauses[19].browsePathSize = 1;
    selectClauses[19].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[19].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[19].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[19].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Prompt");

    /* ResponseOptionSet */
    selectClauses[20].browsePathSize = 1;
    selectClauses[20].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[20].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[20].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[20].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "ResponseOptionSet");

    /* DefaultResponse */
    selectClauses[21].browsePathSize = 1;
    selectClauses[21].browsePath = (UA_QualifiedName*)
                UA_Array_new(selectClauses[21].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[21].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[21].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "DefaultResponse");

    /* DefaultResponse */
    selectClauses[22].browsePathSize = 2;
    selectClauses[22].browsePath = (UA_QualifiedName*)
    UA_Array_new(selectClauses[22].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[22].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[22].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "DialogState");
    selectClauses[22].browsePath[1] = UA_QUALIFIEDNAME_ALLOC(0, "Id");

    selectClauses[23].browsePathSize = 0;
    selectClauses[23].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    selectClauses[23].attributeId = UA_ATTRIBUTEID_NODEID;

    return selectClauses;
}
#endif
