/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef UA_DRIVER_ALARMS_CONDITIONS_H_
#define UA_DRIVER_ALARMS_CONDITIONS_H_

#include <open62541/server.h>

/**
 * Alarms & Conditions Driver (Experimental)
 * -----------------------------------------
 *
 * OPC UA Alarms & Conditions model process states that need operator
 * attention: limit violations, abnormal device states, certificate expiry, and
 * similar situations. Unlike plain events, conditions are stateful. They can
 * remain active, be acknowledged or confirmed, carry comments, and be
 * refreshed for clients that reconnect or subscribe later.
 *
 * The central concept is the Condition instance. A Condition is an Object of a
 * ConditionType, such as OffNormalAlarmType or ExclusiveLimitAlarmType. It is
 * associated with a Condition Source, the node whose state is represented by
 * the Condition. The source is connected to the event hierarchy with
 * HasEventSource so clients can discover and subscribe to relevant alarms.
 *
 * Conditions expose standardized fields such as EnabledState, ActiveState,
 * AckedState, ConfirmedState, Severity, Message, Retain, and SourceNode. These
 * fields describe the current alarm state and are also copied into emitted
 * condition events. Retain marks conditions that are still relevant to a
 * client. Acknowledgement and confirmation model operator interaction.
 *
 * This driver provides the server-side bookkeeping for Conditions, retained
 * EventIds, refresh handling, state callbacks, and helper functions to create,
 * update, trigger, and delete Conditions. Create the driver with
 * UA_AlarmsConditionsDriver() and attach it to a server with
 * UA_Server_addDriver(). Only one Alarms & Conditions driver instance can be
 * attached to a server. */

#if defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)

typedef struct UA_AlarmConditionsDriver UA_AlarmConditionsDriver;

typedef enum UA_TwoStateVariableCallbackType {
    UA_ENTERING_ENABLEDSTATE,
    UA_ENTERING_ACKEDSTATE,
    UA_ENTERING_CONFIRMEDSTATE,
    UA_ENTERING_ACTIVESTATE
} UA_TwoStateVariableCallbackType;

/* User-defined callback for changes of TwoStateVariable fields. */
typedef UA_StatusCode
(*UA_TwoStateVariableChangeCallback)(UA_Server *server,
                                     const UA_NodeId *condition);

struct UA_AlarmConditionsDriver {
    UA_Driver drv; /* Must be the first member */

    /* Create a condition instance.
     *
     * The conditionType must be a subtype of ConditionType. If the condition
     * source has no inverse HasEventSource reference, a HasEventSource
     * reference from the Server object to the source is created. Pass a
     * hierarchical reference type to expose the condition below its source in
     * the address space. Pass UA_NODEID_NULL to keep the condition unexposed.
     *
     * @param driver The Alarms & Conditions driver
     * @param conditionId The requested Condition Object NodeId. Passing
     *        UA_NODEID_NUMERIC(X,0) selects an unused NodeId in namespace X.
     *        Passing UA_NODEID_NULL selects an unused NodeId in namespace 0.
     * @param conditionType The ConditionType node
     * @param conditionName The condition name
     * @param conditionSource The Condition Source node
     * @param hierarchialReferenceType The hierarchical ReferenceType between
     *        the condition source and the condition
     * @param outConditionId The created Condition node
     * @return The StatusCode of the operation */
    UA_StatusCode (*createCondition)(UA_AlarmConditionsDriver *driver,
                                     const UA_NodeId conditionId,
                                     const UA_NodeId conditionType,
                                     const UA_QualifiedName conditionName,
                                     const UA_NodeId conditionSource,
                                     const UA_NodeId hierarchialReferenceType,
                                     UA_NodeId *outConditionId);

    /* Begin creating a condition instance.
     *
     * This is the first half of createCondition(), mirroring the
     * UA_Server_addNode_begin() / UA_Server_addNode_finish() pattern. Use it
     * when the condition node shall be modified before instantiation is
     * finished, for example to add children with specific NodeIds.
     *
     * @param driver The Alarms & Conditions driver
     * @param conditionId The requested Condition Object NodeId. Passing
     *        UA_NODEID_NUMERIC(X,0) selects an unused NodeId in namespace X.
     *        Passing UA_NODEID_NULL selects an unused NodeId in namespace 0.
     * @param conditionType The ConditionType node. It must be a subtype of the
     *        OPC UA ConditionType.
     * @param conditionName The condition name
     * @param outConditionId The added Condition node
     * @return The StatusCode of the operation */
    UA_StatusCode (*addConditionBegin)(UA_AlarmConditionsDriver *driver,
                                       const UA_NodeId conditionId,
                                       const UA_NodeId conditionType,
                                       const UA_QualifiedName conditionName,
                                       UA_NodeId *outConditionId);

    /* Finish creating a condition instance.
     *
     * This is the second half of the addConditionBegin() /
     * addConditionFinish() pair. It finishes the node, creates missing
     * HasEventSource references, optionally exposes the condition below its
     * source, and initializes the standard condition fields and callbacks.
     *
     * @param driver The Alarms & Conditions driver
     * @param conditionId The unfinished Condition Object node
     * @param conditionSource The Condition Source node
     * @param hierarchialReferenceType The hierarchical ReferenceType between
     *        the condition source and the condition
     * @return The StatusCode of the operation */
    UA_StatusCode (*addConditionFinish)(UA_AlarmConditionsDriver *driver,
                                        const UA_NodeId conditionId,
                                        const UA_NodeId conditionSource,
                                        const UA_NodeId hierarchialReferenceType);

    /* Set the value of a condition field.
     *
     * @param driver The Alarms & Conditions driver
     * @param condition The Condition instance node
     * @param value The value to write
     * @param fieldName The target field name
     * @return The StatusCode of the operation */
    UA_StatusCode (*setConditionField)(UA_AlarmConditionsDriver *driver,
                                       const UA_NodeId condition,
                                       const UA_Variant *value,
                                       const UA_QualifiedName fieldName);

    /* Set a property value of a condition field.
     *
     * @param driver The Alarms & Conditions driver
     * @param condition The Condition instance node
     * @param value The value to write
     * @param variableFieldName The field containing the property
     * @param variablePropertyName The target field property name
     * @return The StatusCode of the operation */
    UA_StatusCode (*setConditionVariableFieldProperty)(
        UA_AlarmConditionsDriver *driver, const UA_NodeId condition,
        const UA_Variant *value,
        const UA_QualifiedName variableFieldName,
        const UA_QualifiedName variablePropertyName);

    /* Trigger an event for an enabled condition.
     *
     * The internal condition state is updated with the generated EventId.
     *
     * @param driver The Alarms & Conditions driver
     * @param condition The Condition instance node
     * @param conditionSource The Condition Source node
     * @param outEventId The generated EventId
     * @return The StatusCode of the operation */
    UA_StatusCode (*triggerConditionEvent)(UA_AlarmConditionsDriver *driver,
                                           const UA_NodeId condition,
                                           const UA_NodeId conditionSource,
                                           UA_ByteString *outEventId);

    /* Add an optional condition field by name.
     *
     * Optional methods are not implemented yet.
     *
     * @param driver The Alarms & Conditions driver
     * @param condition The Condition instance node
     * @param conditionType The ConditionType containing the optional field
     * @param fieldName The optional field name
     * @param outOptionalVariable The created field Variable node
     * @return The StatusCode of the operation */
    UA_StatusCode (*addConditionOptionalField)(UA_AlarmConditionsDriver *driver,
                                               const UA_NodeId condition,
                                               const UA_NodeId conditionType,
                                               const UA_QualifiedName fieldName,
                                               UA_NodeId *outOptionalVariable);

    /* Set a callback for a condition TwoStateVariable field.
     *
     * The callback is called before triggering events when EnabledState/Id,
     * AckedState/Id, ConfirmedState/Id, or ActiveState/Id transitions to true.
     *
     * @param driver The Alarms & Conditions driver
     * @param condition The Condition instance node
     * @param conditionSource The Condition Source node
     * @param removeBranch Not implemented yet
     * @param callback The user callback
     * @param callbackType The state transition for which the callback is used
     * @return The StatusCode of the operation */
    UA_StatusCode (*setConditionTwoStateVariableCallback)(
        UA_AlarmConditionsDriver *driver, const UA_NodeId condition,
        const UA_NodeId conditionSource, UA_Boolean removeBranch,
        UA_TwoStateVariableChangeCallback callback,
        UA_TwoStateVariableCallbackType callbackType);

    /* Delete a condition from the address space and internal state.
     *
     * @param driver The Alarms & Conditions driver
     * @param condition The Condition instance node
     * @param conditionSource The Condition Source node
     * @return The StatusCode of the operation */
    UA_StatusCode (*deleteCondition)(UA_AlarmConditionsDriver *driver,
                                     const UA_NodeId condition,
                                     const UA_NodeId conditionSource);

    /* Set the LimitState of a LimitAlarmType condition.
     *
     * @param driver The Alarms & Conditions driver
     * @param conditionId The Condition instance node
     * @param limitValue The value from the trigger node
     * @return The StatusCode of the operation */
    UA_StatusCode (*setLimitState)(UA_AlarmConditionsDriver *driver,
                                   const UA_NodeId conditionId,
                                   UA_Double limitValue);

    /* Parse a certificate and set the ExpirationDate.
     *
     * @param driver The Alarms & Conditions driver
     * @param conditionId The Condition instance node
     * @param cert The certificate to parse
     * @return The StatusCode of the operation */
    UA_StatusCode (*setExpirationDate)(UA_AlarmConditionsDriver *driver,
                                       const UA_NodeId conditionId,
                                       UA_ByteString cert);
};

UA_EXPORT UA_AlarmConditionsDriver *
UA_AlarmsConditionsDriver(const UA_KeyValueMap params);

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS && UA_GENERATED_NAMESPACE_ZERO_FULL */

#endif /* UA_DRIVER_ALARMS_CONDITIONS_H_ */
