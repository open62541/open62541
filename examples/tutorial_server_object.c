/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Working with Objects and Object Types
 * -------------------------------------
 *
 * Using objects to structure information models
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Assume a situation where we want to model a set of pumps and their runtime
 * state in an OPC UA information model. Of course, all pump representations
 * should follow the same basic structure, For example, we might have graphical
 * representation of pumps in a SCADA visualisation that shall be resuable for
 * all pumps.
 *
 * Following the object-oriented programming paradigm, every pump is represented
 * by an object with the following layout:
 *
 * .. graphviz::
 *
 *    digraph tree {
 *
 *    fixedsize=true;
 *    node [width=2, height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]
 *
 *    node_root [label=< <I>ObjectNode</I><BR/>Pump >]
 *
 *    { rank=same
 *      point_1 [shape=point]
 *      node_1 [label=< <I>VariableNode</I><BR/>ManufacturerName >] }
 *    node_root -> point_1 [arrowhead=none]
 *    point_1 -> node_1 [label="hasComponent"]
 *
 *    { rank=same
 *      point_2 [shape=point]
 *      node_2 [label=< <I>VariableNode</I><BR/>ModelName >] }
 *    point_1 -> point_2 [arrowhead=none]
 *    point_2 -> node_2 [label="hasComponent"]
 *
 *    {  rank=same
 *       point_4 [shape=point]
 *       node_4 [label=< <I>VariableNode</I><BR/>Status >] }
 *    point_2 -> point_4 [arrowhead=none]
 *    point_4 -> node_4 [label="hasComponent"]
 *
 *    {  rank=same
 *       point_5 [shape=point]
 *       node_5 [label=< <I>VariableNode</I><BR/>MotorRPM >] }
 *    point_4 -> point_5 [arrowhead=none]
 *    point_5 -> node_5 [label="hasComponent"]
 *
 *    }
 *
 * The following code manually defines a pump and its member variables. We omit
 * setting constraints on the variable values as this is not the focus of this
 * tutorial and was already covered. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>

static void
manuallyDefinePump(UA_Server *server) {
    UA_NodeId pumpId; /* get the nodeid assigned by the server */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Pump (Manual)");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Pump (Manual)"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &pumpId);

    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    UA_String manufacturerName = UA_STRING("Pump King Ltd.");
    UA_Variant_setScalar(&mnAttr.value, &manufacturerName, &UA_TYPES[UA_TYPES_STRING]);
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ManufacturerName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), mnAttr, NULL, NULL);

    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    UA_String modelName = UA_STRING("Mega Pump 3000");
    UA_Variant_setScalar(&modelAttr.value, &modelName, &UA_TYPES[UA_TYPES_STRING]);
    modelAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ModelName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ModelName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), modelAttr, NULL, NULL);

    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    UA_Boolean status = true;
    UA_Variant_setScalar(&statusAttr.value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Status"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, NULL);

    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    UA_Double rpm = 50.0;
    UA_Variant_setScalar(&rpmAttr.value, &rpm, &UA_TYPES[UA_TYPES_DOUBLE]);
    rpmAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorRPM");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MotorRPMs"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), rpmAttr, NULL, NULL);
}

/**
 * Object types, type hierarchies and instantiation
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Building up each object manually requires us to write a lot of code.
 * Furthermore, there is no way for clients to detect that an object represents
 * a pump. (We might use naming conventions or similar to detect pumps. But
 * that's not exactly a clean solution.) Furthermore, we might have more devices
 * than just pumps. And we require all devices to share some common structure.
 * The solution is to define ObjectTypes in a hierarchy with inheritance
 * relations.
 *
 * .. graphviz::
 *
 *    digraph tree {
 *
 *    fixedsize=true;
 *    node [width=2, height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]
 *
 *    node_root [label=< <I>ObjectTypeNode</I><BR/>Device >]
 *
 *    { rank=same
 *      point_1 [shape=point]
 *      node_1 [label=< <I>VariableNode</I><BR/>ManufacturerName<BR/>(mandatory) >] }
 *    node_root -> point_1 [arrowhead=none]
 *    point_1 -> node_1 [label="hasComponent"]
 *
 *    { rank=same
 *      point_2 [shape=point]
 *      node_2 [label=< <I>VariableNode</I><BR/>ModelName >] }
 *    point_1 -> point_2 [arrowhead=none]
 *    point_2 -> node_2 [label="hasComponent"]
 *
 *    {  rank=same
 *       point_3 [shape=point]
 *       node_3 [label=< <I>ObjectTypeNode</I><BR/>Pump >] }
 *    point_2 -> point_3 [arrowhead=none]
 *    point_3 -> node_3 [label="hasSubtype"]
 *
 *    {  rank=same
 *       point_4 [shape=point]
 *       node_4 [label=< <I>VariableNode</I><BR/>Status<BR/>(mandatory) >] }
 *    node_3 -> point_4 [arrowhead=none]
 *    point_4 -> node_4 [label="hasComponent"]
 *
 *    {  rank=same
 *       point_5 [shape=point]
 *       node_5 [label=< <I>VariableNode</I><BR/>MotorRPM >] }
 *    point_4 -> point_5 [arrowhead=none]
 *    point_5 -> node_5 [label="hasComponent"]
 *
 *    }
 *
 * Children that are marked mandatory are automatically instantiated together
 * with the parent object. This is indicated by a `hasModellingRule` reference
 * to an object that representes the `mandatory` modelling rule. */

/* predefined identifier for later use */
UA_NodeId pumpTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static void
defineObjectTypes(UA_Server *server) {
    /* Define the object type for "Device" */
    UA_NodeId deviceTypeId; /* get the nodeid assigned by the server */
    UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DeviceType"), dtAttr,
                                NULL, &deviceTypeId);

    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_NodeId manufacturerNameId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ManufacturerName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), mnAttr, NULL, &manufacturerNameId);
    /* Make the manufacturer name mandatory */
    UA_Server_addReference(server, manufacturerNameId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);


    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    modelAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ModelName");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "ModelName"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), modelAttr, NULL, NULL);

    /* Define the object type for "Pump" */
    UA_ObjectTypeAttributes ptAttr = UA_ObjectTypeAttributes_default;
    ptAttr.displayName = UA_LOCALIZEDTEXT("en-US", "PumpType");
    UA_Server_addObjectTypeNode(server, pumpTypeId,
                                deviceTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "PumpType"), ptAttr,
                                NULL, NULL);

    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    statusAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId statusId;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Status"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), statusAttr, NULL, &statusId);
    /* Make the status variable mandatory */
    UA_Server_addReference(server, statusId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    rpmAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorRPM");
    rpmAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "MotorRPMs"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), rpmAttr, NULL, NULL);
}

/**
 * Now we add the derived ObjectType for the pump that inherits from the device
 * object type. The resulting object contains all mandatory child variables.
 * These are simply copied over from the object type. The object has a reference
 * of type ``hasTypeDefinition`` to the object type, so that clients can detect
 * the type-instance relation at runtime.
 */

static void
addPumpObjectInstance(UA_Server *server, char *name) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, name),
                            pumpTypeId, /* this refers to the object type
                                           identifier */
                            oAttr, NULL, NULL);
}

/**
 * Often times, we want to run a constructor function on a new object. This is
 * especially useful when an object is instantiated at runtime (with the
 * AddNodes service) and the integration with an underlying process canot be
 * manually defined. In the following constructor example, we simply set the
 * pump status to on.
 */

static UA_StatusCode
pumpTypeConstructor(UA_Server *server,
                    const UA_NodeId *sessionId, void *sessionContext,
                    const UA_NodeId *typeId, void *typeContext,
                    const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New pump created");

    /* Find the NodeId of the status child variable */
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = UA_QUALIFIEDNAME(1, "Status");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *nodeId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr =
        UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return bpr.statusCode;

    /* Set the status value */
    UA_Boolean status = true;
    UA_Variant value;
    UA_Variant_setScalar(&value, &status, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bpr);

    /* At this point we could replace the node context .. */

    return UA_STATUSCODE_GOOD;
}

static void
addPumpTypeConstructor(UA_Server *server) {
    UA_NodeTypeLifecycle lifecycle;
    lifecycle.constructor = pumpTypeConstructor;
    lifecycle.destructor = NULL;
    UA_Server_setNodeTypeLifecycle(server, pumpTypeId, lifecycle);
}

/** It follows the main server code, making use of the above definitions. */

static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    manuallyDefinePump(server);
    defineObjectTypes(server);
    addPumpObjectInstance(server, "pump2");
    addPumpObjectInstance(server, "pump3");
    addPumpTypeConstructor(server);
    addPumpObjectInstance(server, "pump4");
    addPumpObjectInstance(server, "pump5");

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
