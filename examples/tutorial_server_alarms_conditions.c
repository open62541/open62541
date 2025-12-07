/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

/**
 * Using Alarms and Conditions Server
 * ----------------------------------
 *
 * Besides the usage of monitored items and events to observe the changes in the
 * server, it is also important to make use of the Alarms and Conditions Server
 * Model. Alarms are events which are triggered automatically by the server
 * dependent on internal server logic or user specific logic when the states of
 * server components change. The state of a component is represented through a
 * condition. So the values of all the condition children (Fields) are the
 * actual state of the component.
 *
 * Trigger Alarm events by changing States
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * The following example will be based on the server events tutorial. Please
 * make sure to understand the principle of normal events before proceeding with
 * this example! */

static UA_NodeId conditionSource;
static UA_NodeId conditionInstance_1;
static UA_NodeId conditionInstance_2;

static UA_StatusCode
addConditionSourceObject(UA_Server *server) {
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.eventNotifier = 1;

    object_attr.displayName = UA_LOCALIZEDTEXT("en", "ConditionSourceObject");
    UA_StatusCode retval =  UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                      UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
                                      UA_QUALIFIEDNAME(0, "ConditionSourceObject"),
                                      UA_NS0ID(BASEOBJECTTYPE),
                                      object_attr, NULL, &conditionSource);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Creating Condition Source failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    /* ConditionSource should be EventNotifier of another Object (usually the
     * Server Object). If this Reference is not created by user then the A&C
     * Server will create "HasEventSource" reference to the Server Object
     * automatically when the condition is created*/
    retval = UA_Server_addReference(server, UA_NS0ID(SERVER), UA_NS0ID(HASNOTIFIER),
                                     UA_EXPANDEDNODEID_NUMERIC(conditionSource.namespaceIndex,
                                                               conditionSource.identifier.numeric),
                                     UA_TRUE);

    return retval;
}

/**
 * Create a condition instance from OffNormalAlarmType. The condition source is
 * the Object created in addConditionSourceObject(). The condition will be
 * exposed in Address Space through the HasComponent reference to the condition
 * source. */
static UA_StatusCode
addCondition_1(UA_Server *server) {
    UA_StatusCode retval = addConditionSourceObject(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "creating Condition Source failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    retval = UA_Server_createCondition(server, UA_NODEID_NULL, UA_NS0ID(OFFNORMALALARMTYPE),
                                       UA_QUALIFIEDNAME(0, "Condition 1"), conditionSource,
                                       UA_NS0ID(HASCOMPONENT), &conditionInstance_1);

    return retval;
}

/**
 * Create a condition instance from OffNormalAlarmType. The condition source is
 * the server Object. The condition won't be exposed in Address Space. */
static UA_StatusCode
addCondition_2(UA_Server *server) {
    UA_StatusCode retval =
        UA_Server_createCondition(server, UA_NODEID_NULL, UA_NS0ID(OFFNORMALALARMTYPE),
                                  UA_QUALIFIEDNAME(0, "Condition 2"), UA_NS0ID(SERVER),
                                  UA_NODEID_NULL, &conditionInstance_2);

    return retval;
}

static void
addVariable_1_triggerAlarmOfCondition_1(UA_Server *server, UA_NodeId* outNodeId) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "Activate Condition 1");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Boolean tboolValue = UA_FALSE;
    UA_Variant_setScalar(&attr.value, &tboolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_QualifiedName CallbackTestVariableName = UA_QUALIFIEDNAME(0, "Activate Condition 1");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NS0ID(BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                              parentReferenceNodeId, CallbackTestVariableName,
                              variableTypeNodeId, attr, NULL, outNodeId);
}

static void
addVariable_2_changeSeverityOfCondition_2(UA_Server *server,
                                          UA_NodeId* outNodeId) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "Change Severity Condition 2");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_UInt16 severityValue = 0;
    UA_Variant_setScalar(&attr.value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);

    UA_QualifiedName CallbackTestVariableName =
        UA_QUALIFIEDNAME(0, "Change Severity Condition 2");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NS0ID(BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                              parentReferenceNodeId, CallbackTestVariableName,
                              variableTypeNodeId, attr, NULL, outNodeId);
}

static void
addVariable_3_returnCondition_1_toNormalState(UA_Server *server,
                                              UA_NodeId* outNodeId) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "Return to Normal Condition 1");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Boolean rtn = 0;
    UA_Variant_setScalar(&attr.value, &rtn, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_QualifiedName CallbackTestVariableName =
        UA_QUALIFIEDNAME(0, "Return to Normal Condition 1");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NS0ID(BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                              parentReferenceNodeId, CallbackTestVariableName,
                              variableTypeNodeId, attr, NULL, outNodeId);
}

static void
afterWriteCallbackVariable_1(UA_Server *server, const UA_NodeId *sessionId,
                             void *sessionContext, const UA_NodeId *nodeId,
                             void *nodeContext, const UA_NumericRange *range,
                             const UA_DataValue *data) {
    UA_QualifiedName activeStateField = UA_QUALIFIEDNAME(0,"ActiveState");
    UA_QualifiedName activeStateIdField = UA_QUALIFIEDNAME(0,"Id");
    UA_Variant value;

    UA_StatusCode retval =
        UA_Server_writeObjectProperty_scalar(server, conditionInstance_1,
                                             UA_QUALIFIEDNAME(0, "Time"),
                                             &data->sourceTimestamp,
                                             &UA_TYPES[UA_TYPES_DATETIME]);

    if(*(UA_Boolean *)(data->value.data) == true) {
        /* By writing "true" in ActiveState/Id, the A&C server will set the
         * related fields automatically and then will trigger event
         * notification. */
        UA_Boolean activeStateId = true;
        UA_Variant_setScalar(&value, &activeStateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval |= UA_Server_setConditionVariableFieldProperty(server, conditionInstance_1,
                                                              &value, activeStateField,
                                                              activeStateIdField);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                         "Setting ActiveState/Id Field failed. StatusCode %s",
                         UA_StatusCode_name(retval));
            return;
        }
    } else {
        /* By writing "false" in ActiveState/Id, the A&C server will set only
         * the ActiveState field automatically to the value "Inactive". The user
         * should trigger the event manually by calling
         * UA_Server_triggerConditionEvent inside the application or call
         * ConditionRefresh method with client to update the event notification. */
        UA_Boolean activeStateId = false;
        UA_Variant_setScalar(&value, &activeStateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval = UA_Server_setConditionVariableFieldProperty(server, conditionInstance_1,
                                                             &value, activeStateField,
                                                             activeStateIdField);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                         "Setting ActiveState/Id Field failed. StatusCode %s",
                         UA_StatusCode_name(retval));
            return;
        }

        retval = UA_Server_triggerConditionEvent(server, conditionInstance_1,
                                                 conditionSource, NULL);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                           "Triggering condition event failed. StatusCode %s",
                           UA_StatusCode_name(retval));
            return;
        }
    }
}

/**
 * The callback only changes the severity field of the condition 2. The severity
 * field is of ConditionVariableType, so changes in it triggers an event
 * notification automatically by the server. */
static void
afterWriteCallbackVariable_2(UA_Server *server, const UA_NodeId *sessionId,
                             void *sessionContext, const UA_NodeId *nodeId,
                             void *nodeContext, const UA_NumericRange *range,
                             const UA_DataValue *data) {
   /* Another way to set fields of conditions */
    UA_Server_writeObjectProperty_scalar(server, conditionInstance_2,
                                         UA_QUALIFIEDNAME(0, "Severity"),
                                         (UA_UInt16 *)data->value.data,
                                         &UA_TYPES[UA_TYPES_UINT16]);
}

/**
 * RTN = return to normal.
 *
 * Retain will be set to false, thus no events will be generated for condition 1
 * (although EnabledState/=true). To set Retain to true again, the disable and
 * enable methods should be called respectively.
 */
static void
afterWriteCallbackVariable_3(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext,
               const UA_NumericRange *range, const UA_DataValue *data) {

    //UA_QualifiedName enabledStateField = UA_QUALIFIEDNAME(0,"EnabledState");
    UA_QualifiedName ackedStateField = UA_QUALIFIEDNAME(0,"AckedState");
    UA_QualifiedName confirmedStateField = UA_QUALIFIEDNAME(0,"ConfirmedState");
    UA_QualifiedName activeStateField = UA_QUALIFIEDNAME(0,"ActiveState");
    UA_QualifiedName severityField = UA_QUALIFIEDNAME(0,"Severity");
    UA_QualifiedName messageField = UA_QUALIFIEDNAME(0,"Message");
    UA_QualifiedName commentField = UA_QUALIFIEDNAME(0,"Comment");
    UA_QualifiedName retainField = UA_QUALIFIEDNAME(0,"Retain");
    UA_QualifiedName idField = UA_QUALIFIEDNAME(0,"Id");

    UA_StatusCode retval =
        UA_Server_writeObjectProperty_scalar(server, conditionInstance_1,
                                             UA_QUALIFIEDNAME(0, "Time"),
                                             &data->serverTimestamp,
                                             &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant value;
    UA_Boolean idValue = false;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval |= UA_Server_setConditionVariableFieldProperty(server, conditionInstance_1,
                                                          &value, activeStateField,
                                                          idField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting ActiveState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    retval = UA_Server_setConditionVariableFieldProperty(server, conditionInstance_1,
                                                         &value, ackedStateField,
                                                         idField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting AckedState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    retval = UA_Server_setConditionVariableFieldProperty(server, conditionInstance_1,
                                                         &value, confirmedStateField,
                                                         idField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting ConfirmedState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_UInt16 severityValue = 100;
    UA_Variant_setScalar(&value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_setConditionField(server, conditionInstance_1,
                                         &value, severityField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting Severity Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_LocalizedText messageValue =
        UA_LOCALIZEDTEXT("en", "Condition returned to normal state");
    UA_Variant_setScalar(&value, &messageValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, conditionInstance_1,
                                         &value, messageField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting Message Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_LocalizedText commentValue = UA_LOCALIZEDTEXT("en", "Normal State");
    UA_Variant_setScalar(&value, &commentValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, conditionInstance_1,
                                         &value, commentField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting Comment Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_Boolean retainValue = false;
    UA_Variant_setScalar(&value, &retainValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(server, conditionInstance_1,
                                         &value, retainField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting Retain Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    retval = UA_Server_triggerConditionEvent(server, conditionInstance_1,
                                             conditionSource, NULL);
    if (retval != UA_STATUSCODE_GOOD) {
     UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Triggering condition event failed. StatusCode %s",
                    UA_StatusCode_name(retval));
     return;
    }
}

static UA_StatusCode
enteringEnabledStateCallback(UA_Server *server, const UA_NodeId *condition) {
    UA_Boolean retain = true;
    return UA_Server_writeObjectProperty_scalar(server, *condition,
                                                UA_QUALIFIEDNAME(0, "Retain"),
                                                &retain,
                                                &UA_TYPES[UA_TYPES_BOOLEAN]);
}

/**
 * This is user specific function which will be called upon acknowledging an
 * alarm notification. In this example we will set the Alarm to Inactive state.
 * The server is responsible of setting standard fields related to Acknowledge
 * Method and triggering the alarm notification. */
static UA_StatusCode
enteringAckedStateCallback(UA_Server *server, const UA_NodeId *condition) {
    /* deactivate Alarm when acknowledging*/
    UA_Boolean activeStateId = false;
    UA_Variant value;
    UA_QualifiedName activeStateField = UA_QUALIFIEDNAME(0,"ActiveState");
    UA_QualifiedName activeStateIdField = UA_QUALIFIEDNAME(0,"Id");

    UA_Variant_setScalar(&value, &activeStateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval =
        UA_Server_setConditionVariableFieldProperty(server, *condition,
                                                    &value, activeStateField,
                                                    activeStateIdField);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting ActiveState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    return retval;
}

static UA_StatusCode
enteringConfirmedStateCallback(UA_Server *server, const UA_NodeId *condition) {
    /* Deactivate Alarm and put it out of the interesting state (by writing
     * false to Retain field) when confirming*/
    UA_Boolean activeStateId = false;
    UA_Boolean retain = false;
    UA_Variant value;
    UA_QualifiedName activeStateField = UA_QUALIFIEDNAME(0,"ActiveState");
    UA_QualifiedName activeStateIdField = UA_QUALIFIEDNAME(0,"Id");
    UA_QualifiedName retainField = UA_QUALIFIEDNAME(0,"Retain");

    UA_Variant_setScalar(&value, &activeStateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval =
        UA_Server_setConditionVariableFieldProperty(server, *condition,
                                                    &value, activeStateField,
                                                    activeStateIdField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting ActiveState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    UA_Variant_setScalar(&value, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(server, *condition,
                                         &value, retainField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting ActiveState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    return retval;
}

static UA_StatusCode
setUpEnvironment(UA_Server *server) {
    UA_NodeId variable_1;
    UA_NodeId variable_2;
    UA_NodeId variable_3;
    UA_ValueCallback callback;
    callback.onRead = NULL;

    /* Exposed condition 1. We will add to it user specific callbacks when
     * entering enabled state, when acknowledging and when confirming. */
    UA_StatusCode retval = addCondition_1(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "adding condition 1 failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    UA_TwoStateVariableChangeCallback userSpecificCallback = enteringEnabledStateCallback;
    retval = UA_Server_setConditionTwoStateVariableCallback(server, conditionInstance_1,
                                                            conditionSource, false,
                                                            userSpecificCallback,
                                                            UA_ENTERING_ENABLEDSTATE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "adding entering enabled state callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    userSpecificCallback = enteringAckedStateCallback;
    retval = UA_Server_setConditionTwoStateVariableCallback(server, conditionInstance_1,
                                                            conditionSource, false,
                                                            userSpecificCallback,
                                                            UA_ENTERING_ACKEDSTATE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "adding entering acked state callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    userSpecificCallback = enteringConfirmedStateCallback;
    retval = UA_Server_setConditionTwoStateVariableCallback(server, conditionInstance_1,
                                                            conditionSource, false,
                                                            userSpecificCallback,
                                                            UA_ENTERING_CONFIRMEDSTATE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "adding entering confirmed state callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    /* Unexposed condition 2. No user specific callbacks, so the server will
     * behave in a standard manner upon entering enabled state, acknowledging
     * and confirming. We will set Retain field to true and enable the condition
     * so we can receive event notifications (we cannot call enable method on
     * unexposed condition using a client like UaExpert or Softing). */
    retval = addCondition_2(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "adding condition 2 failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    UA_Boolean retain = UA_TRUE;
    UA_Server_writeObjectProperty_scalar(server, conditionInstance_2,
                                         UA_QUALIFIEDNAME(0, "Retain"),
                                         &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_Variant value;
    UA_Boolean enabledStateId = true;
    UA_QualifiedName enabledStateField = UA_QUALIFIEDNAME(0,"EnabledState");
    UA_QualifiedName enabledStateIdField = UA_QUALIFIEDNAME(0,"Id");
    UA_Variant_setScalar(&value, &enabledStateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(server, conditionInstance_2,
                                                         &value, enabledStateField,
                                                         enabledStateIdField);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting EnabledState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }


    /* Add 3 variables to trigger condition events */
    addVariable_1_triggerAlarmOfCondition_1(server, &variable_1);

    callback.onWrite = afterWriteCallbackVariable_1;
    retval = UA_Server_setVariableNode_valueCallback(server, variable_1, callback);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting variable 1 Callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    /* Severity can change internally also when the condition disabled and
     * retain is false. However, in this case no events will be generated. */
    addVariable_2_changeSeverityOfCondition_2(server, &variable_2);

    callback.onWrite = afterWriteCallbackVariable_2;
    retval = UA_Server_setVariableNode_valueCallback(server, variable_2, callback);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting variable 2 Callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    addVariable_3_returnCondition_1_toNormalState(server, &variable_3);

    callback.onWrite = afterWriteCallbackVariable_3;
    retval = UA_Server_setVariableNode_valueCallback(server, variable_3, callback);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Setting variable 3 Callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    return retval;
}

/**
 * It follows the main server code, making use of the above definitions. */

int main (void) {
    UA_Server *server = UA_Server_new();

    setUpEnvironment(server);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
