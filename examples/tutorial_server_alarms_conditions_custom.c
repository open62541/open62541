/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

/**
 * Using Custom Alarms and Conditions
 * ----------------------------------
 *
 * Sometimes the application needs to have full control over the nodes, like creating 
 * all nodes manually to manage the memory and NodeIds. This example will demonstrate
 * adding alarms and conditions behavior to condition objects, which where created in 
 * custom manner. For simplicity, UA_Server_addObjectNode will be used to create the
 * condition object itself and all its children nodes. Condition refresh event objects
 * must be created manually in this case. Condition properties should be set manually 
 * as well (not demonstrated in this example).
 *
 * Note: The following example will be based on the server events and alarms and conditions 
 * tutorials. Please make sure to understand the principle of normal events and alarms and 
 * conditions before proceeding with this example! */

static UA_NodeId conditionSource;
static UA_NodeId customConditionInstance;
static UA_NodeId conditionRefreshStart;
static UA_NodeId conditionRefreshEnd;

/**
 * Create condition source object */
static UA_StatusCode
addConditionSourceObject(UA_Server *server) {
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.eventNotifier = 1;

    object_attr.displayName = UA_LOCALIZEDTEXT("en", "ConditionSourceObject");
    UA_StatusCode retval =  UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                    UA_QUALIFIEDNAME(0, "ConditionSourceObject"),
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                    object_attr, NULL, &conditionSource);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Creating Condition Source failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    /* ConditionSource should be EventNotifier of another Object (usually the
     * Server Object). If this Reference is not created by user then the A&C
     * Server will create "HasEventSource" reference to the Server Object
     * automatically when the condition is created*/
    retval = UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASNOTIFIER),
                                     UA_EXPANDEDNODEID_NUMERIC(conditionSource.namespaceIndex,
                                                               conditionSource.identifier.numeric),
                                     UA_TRUE);

    return retval;
}

/**
 * Create a custom condition instance (as normal object) from OffNormalAlarmType. The condition source is
 * the Object created in addConditionSourceObject(). The condition will be
 * exposed in Address Space through the HasComponent reference to the condition
 * source. */
static UA_StatusCode
addCustomCondition(UA_Server *server) {
    UA_StatusCode retval = addConditionSourceObject(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "creating Condition Source failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("en", "CustomCondition");
    retval =  UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                      conditionSource,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                      UA_QUALIFIEDNAME(0, "CustomCondition"),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
                                      object_attr, NULL, &customConditionInstance);
    return retval;
}

/**
 * attach condition behavior to a custom condition instance */
static UA_StatusCode
attachConditionBehavior(UA_Server *server) {
    UA_StatusCode retval =
        UA_Server_attachConditionBehavior(server, customConditionInstance,
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
                                          UA_QUALIFIEDNAME(0, "CustomCondition"),
                                          conditionSource,
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), 
                                          conditionRefreshStart,
                                          conditionRefreshEnd);

    return retval;
}

/**
 * Create Condition Refresh Event Objects */
static UA_StatusCode
addConditionRefreshEvents(UA_Server *server) {
    UA_StatusCode retval = UA_Server_createEvent(server, 
                                                 UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHSTARTEVENTTYPE), 
                                                 &conditionRefreshStart);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Creating Condition Refresh Start Event failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    if(retval == UA_STATUSCODE_GOOD) {
        retval = UA_Server_createEvent(server,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHENDEVENTTYPE),
                                       &conditionRefreshEnd);

      if(retval != UA_STATUSCODE_GOOD) {
          UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Creating Condition Refresh End Event failed. StatusCode %s",
                       UA_StatusCode_name(retval));
      }
    }

    return retval;
}

static void
addVariable_1_triggerAlarmOfCondition(UA_Server *server, UA_NodeId* outNodeId) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "Activate Custom Condition");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Boolean tboolValue = UA_FALSE;
    UA_Variant_setScalar(&attr.value, &tboolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_QualifiedName CallbackTestVariableName = UA_QUALIFIEDNAME(0, "Activate Custom Condition");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                              parentReferenceNodeId, CallbackTestVariableName,
                              variableTypeNodeId, attr, NULL, outNodeId);
}

static void
addVariable_3_returnCondition_toNormalState(UA_Server *server,
                                              UA_NodeId* outNodeId) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "Return to Normal Condition");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Boolean rtn = 0;
    UA_Variant_setScalar(&attr.value, &rtn, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_QualifiedName CallbackTestVariableName =
        UA_QUALIFIEDNAME(0, "Return to Normal Condition");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
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
        UA_Server_writeObjectProperty_scalar(server, customConditionInstance,
                                             UA_QUALIFIEDNAME(0, "Time"),
                                             &data->sourceTimestamp,
                                             &UA_TYPES[UA_TYPES_DATETIME]);

    if(*(UA_Boolean *)(data->value.data) == true) {
        /* By writing "true" in ActiveState/Id, the A&C server will set the
         * related fields automatically and then will trigger event
         * notification. */
        UA_Boolean activeStateId = true;
        UA_Variant_setScalar(&value, &activeStateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval |= UA_Server_setConditionVariableFieldProperty(server, customConditionInstance,
                                                              &value, activeStateField,
                                                              activeStateIdField);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
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
        retval = UA_Server_setConditionVariableFieldProperty(server, customConditionInstance,
                                                             &value, activeStateField,
                                                             activeStateIdField);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Setting ActiveState/Id Field failed. StatusCode %s",
                         UA_StatusCode_name(retval));
            return;
        }

        retval = UA_Server_triggerConditionEvent(server, customConditionInstance,
                                                 conditionSource, NULL);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Triggering condition event failed. StatusCode %s",
                           UA_StatusCode_name(retval));
            return;
        }
    }
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
        UA_Server_writeObjectProperty_scalar(server, customConditionInstance,
                                             UA_QUALIFIEDNAME(0, "Time"),
                                             &data->serverTimestamp,
                                             &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant value;
    UA_Boolean idValue = false;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval |= UA_Server_setConditionVariableFieldProperty(server, customConditionInstance,
                                                          &value, activeStateField,
                                                          idField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting ActiveState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    retval = UA_Server_setConditionVariableFieldProperty(server, customConditionInstance,
                                                         &value, ackedStateField,
                                                         idField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting AckedState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    retval = UA_Server_setConditionVariableFieldProperty(server, customConditionInstance,
                                                         &value, confirmedStateField,
                                                         idField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting ConfirmedState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_UInt16 severityValue = 100;
    UA_Variant_setScalar(&value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_setConditionField(server, customConditionInstance,
                                         &value, severityField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting Severity Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_LocalizedText messageValue =
        UA_LOCALIZEDTEXT("en", "Condition returned to normal state");
    UA_Variant_setScalar(&value, &messageValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, customConditionInstance,
                                         &value, messageField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting Message Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_LocalizedText commentValue = UA_LOCALIZEDTEXT("en", "Normal State");
    UA_Variant_setScalar(&value, &commentValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, customConditionInstance,
                                         &value, commentField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting Comment Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    UA_Boolean retainValue = false;
    UA_Variant_setScalar(&value, &retainValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(server, customConditionInstance,
                                         &value, retainField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting Retain Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return;
    }

    retval = UA_Server_triggerConditionEvent(server, customConditionInstance,
                                             conditionSource, NULL);
    if (retval != UA_STATUSCODE_GOOD) {
     UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
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
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
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
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting ActiveState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    UA_Variant_setScalar(&value, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(server, *condition,
                                         &value, retainField);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting ActiveState/Id Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    return retval;
}

static UA_StatusCode
setUpEnvironment(UA_Server *server) {
    UA_NodeId variable_1;
    UA_NodeId variable_3;
    UA_ValueCallback callback;
    callback.onRead = NULL;

    /* Create exposed custom condition */
    UA_StatusCode retval = addCustomCondition(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "adding custom condition failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    /* Create condition refresh events */
    retval = addConditionRefreshEvents(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "creating condition refresh events failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    /* Attach condition behavior to cutom condition node */
    retval = attachConditionBehavior(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "attaching condition behavior to custom condition failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    /* Configure condition callbacks */
    UA_TwoStateVariableChangeCallback userSpecificCallback = enteringEnabledStateCallback;
    retval = UA_Server_setConditionTwoStateVariableCallback(server, customConditionInstance,
                                                            conditionSource, false,
                                                            userSpecificCallback,
                                                            UA_ENTERING_ENABLEDSTATE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "adding entering enabled state callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    userSpecificCallback = enteringAckedStateCallback;
    retval = UA_Server_setConditionTwoStateVariableCallback(server, customConditionInstance,
                                                            conditionSource, false,
                                                            userSpecificCallback,
                                                            UA_ENTERING_ACKEDSTATE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "adding entering acked state callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    userSpecificCallback = enteringConfirmedStateCallback;
    retval = UA_Server_setConditionTwoStateVariableCallback(server, customConditionInstance,
                                                            conditionSource, false,
                                                            userSpecificCallback,
                                                            UA_ENTERING_CONFIRMEDSTATE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "adding entering confirmed state callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    /* Add variables to trigger condition events */
    addVariable_1_triggerAlarmOfCondition(server, &variable_1);

    callback.onWrite = afterWriteCallbackVariable_1;
    retval = UA_Server_setVariableNode_valueCallback(server, variable_1, callback);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting variable 1 Callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    addVariable_3_returnCondition_toNormalState(server, &variable_3);

    callback.onWrite = afterWriteCallbackVariable_3;
    retval = UA_Server_setVariableNode_valueCallback(server, variable_3, callback);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Setting variable 3 Callback failed. StatusCode %s",
                     UA_StatusCode_name(retval));
    }

    return retval;
}

/**
 * It follows the main server code, making use of the above definitions. */

static UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main (void) {
    /* default server values */
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval = setUpEnvironment(server);

    if(retval == UA_STATUSCODE_GOOD)
        retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
