/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

#include <stdio.h>

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

static void
afterInputNodeWrite (UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *nodeId, void *nodeContext,
                     const UA_NumericRange *range, const UA_DataValue *data)
{
    UA_Double input = *(UA_Double*) data->value.data;
    UA_Server_exclusiveLimitAlarmEvaluate(server, &conditionInstance_1, &input);
}

static void *
sourceNodeGetInputDouble (UA_Server *server, const UA_NodeId *conditionId, void *conditionCtx)
{
    UA_Variant val;
    UA_Variant_init (&val);
    UA_StatusCode ret = UA_Server_Condition_getInputNodeValue(server, *conditionId, &val);
    if (ret != UA_STATUSCODE_GOOD || val.type != &UA_TYPES[UA_TYPES_DOUBLE])
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Getting input value from condition InputNode failed. StatusCode %s", UA_StatusCode_name(ret));
        UA_Variant_clear(&val);
        return NULL;
    }
    UA_Double *doubleVal = (UA_Double *) val.data;
    val.data = NULL;
    UA_Variant_clear(&val);
    return doubleVal;
}

static void
sourceNodeInputFreeDouble (void *input, void *conditionCtx)
{
    UA_Double_delete((UA_Double *) input);
}

static UA_StatusCode
addExclusiveLimitAlarmCondition (UA_Server *server) {

    UA_NodeId inputNode;
    UA_Double val = 100;
    UA_VariableAttributes vatt = UA_VariableAttributes_default;
    vatt.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_DOUBLE);
    UA_Variant_setScalar(&vatt.value, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
    vatt.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_StatusCode retval = UA_Server_addVariableNode(
        server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(0, "InputNode"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vatt,
        NULL,
        &inputNode
    );

    UA_ValueCallback callback;
    callback.onRead = NULL;
    callback.onWrite = afterInputNodeWrite;
    UA_Server_setVariableNode_valueCallback (server, inputNode, callback);

    UA_ConditionProperties properties = {
        .source = conditionSource,
        .name = UA_QUALIFIEDNAME(0, "Exculsive Limit Alarm Condition "),
        .hierarchialReferenceType =
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT)
    };

    UA_Duration test = 20.0f;
    UA_Duration reAlarmTime = 1000;
    UA_Int16 repeatCount = 1000;

    UA_AlarmConditionProperties baseP = {
        .inputNode = inputNode,
        .isLatching = true,
        .isShelvable = true
        //.suppressible = true,
        //.serviceable = true,
        //.maxTimeShelved = &test,
        //.onDelay = &test,
        //.offDelay = &test,
        //.reAlarmRepeatCount = &repeatCount,
        //.reAlarmTime = &reAlarmTime

    };

    UA_Double highLimit = 20.0f;
    UA_LimitAlarmProperties alarmProperties = {
        .alarmConditionProperties = baseP,
        .highLimit = &highLimit
    };

    UA_ConditionInputFns inputFns;
    inputFns.getInput = sourceNodeGetInputDouble;
    inputFns.inputFree = sourceNodeInputFreeDouble;

    retval = UA_Server_createExclusiveLimitAlarm (
        server,
        UA_NODEID_NULL,
        &properties,
        inputFns,
        &alarmProperties,
        &conditionInstance_1
    );

    retval = UA_Server_Condition_enable (server, conditionInstance_1, true);
    return retval;
}

static void
setUpEnvironment(UA_Server *server) {
    addConditionSourceObject(server);
    addExclusiveLimitAlarmCondition(server);
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
