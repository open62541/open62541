/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_SERVER_NODES_H_
#define UA_SERVER_NODES_H_

/* !!! Warning !!!
 *
 * If you are not developing a nodestore plugin, then you should not work with
 * the definitions from this file directly. The underlying node structures are
 * not meant to be used directly by end users. Please use the public server API
 * / OPC UA services to interact with the information model. */

#include <open62541/server.h>

_UA_BEGIN_DECLS

/* Forward declaration */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
struct UA_MonitoredItem;
#endif

/**
 * .. _information-modelling:
 *
 * Information Modelling
 * =====================
 *
 * Information modelling in OPC UA combines concepts from object-orientation and
 * semantic modelling. At the core, an OPC UA information model is a graph made
 * up of
 *
 * - Nodes: There are eight possible Node types (variable, object, method, ...)
 * - References: Typed and directed relations between two nodes
 *
 * Every node is identified by a unique (within the server) :ref:`nodeid`.
 * Reference are triples of the form ``(source-nodeid, referencetype-nodeid,
 * target-nodeid)``. An example reference between nodes is a
 * ``hasTypeDefinition`` reference between a Variable and its VariableType. Some
 * ReferenceTypes are *hierarchic* and must not form *directed loops*. See the
 * section on :ref:`ReferenceTypes <referencetypenode>` for more details on
 * possible references and their semantics.
 *
 * **Warning!!** The structures defined in this section are only relevant for
 * the developers of custom Nodestores. The interaction with the information
 * model is possible only via the OPC UA :ref:`services`. So the following
 * sections are purely informational so that users may have a clear mental
 * model of the underlying representation.
 *
 * Base Node Attributes
 * --------------------
 *
 * Nodes contain attributes according to their node type. The base node
 * attributes are common to all node types. In the OPC UA :ref:`services`,
 * attributes are referred to via the :ref:`nodeid` of the containing node and
 * an integer :ref:`attribute-id`.
 *
 * Internally, open62541 uses ``UA_Node`` in places where the exact node type is
 * not known or not important. The ``nodeClass`` attribute is used to ensure the
 * correctness of casting from ``UA_Node`` to a specific node type. */

/* List of reference targets with the same reference type and direction */
typedef struct {
    UA_NodeId referenceTypeId;
    UA_Boolean isInverse;
    size_t targetIdsSize;
    UA_ExpandedNodeId *targetIds;
} UA_NodeReferenceKind;

#define UA_NODE_BASEATTRIBUTES                  \
    UA_NodeId nodeId;                           \
    UA_NodeClass nodeClass;                     \
    UA_QualifiedName browseName;                \
    UA_LocalizedText displayName;               \
    UA_LocalizedText description;               \
    UA_UInt32 writeMask;                        \
    size_t referencesSize;                      \
    UA_NodeReferenceKind *references;           \
                                                \
    /* Members specific to open62541 */         \
    void *context;                              \
    UA_Boolean constructed; /* Constructors were called */

typedef struct {
    UA_NODE_BASEATTRIBUTES
} UA_Node;

/**
 * VariableNode
 * ------------
 *
 * Variables store values in a :ref:`datavalue` together with
 * metadata for introspection. Most notably, the attributes data type, value
 * rank and array dimensions constrain the possible values the variable can take
 * on.
 *
 * Variables come in two flavours: properties and datavariables. Properties are
 * related to a parent with a ``hasProperty`` reference and may not have child
 * nodes themselves. Datavariables may contain properties (``hasProperty``) and
 * also datavariables (``hasComponents``).
 *
 * All variables are instances of some :ref:`variabletypenode` in return
 * constraining the possible data type, value rank and array dimensions
 * attributes.
 *
 * Data Type
 * ^^^^^^^^^
 *
 * The (scalar) data type of the variable is constrained to be of a specific
 * type or one of its children in the type hierarchy. The data type is given as
 * a NodeId pointing to a :ref:`datatypenode` in the type hierarchy. See the
 * Section :ref:`datatypenode` for more details.
 *
 * If the data type attribute points to ``UInt32``, then the value attribute
 * must be of that exact type since ``UInt32`` does not have children in the
 * type hierarchy. If the data type attribute points ``Number``, then the type
 * of the value attribute may still be ``UInt32``, but also ``Float`` or
 * ``Byte``.
 *
 * Consistency between the data type attribute in the variable and its
 * :ref:`VariableTypeNode` is ensured.
 *
 * Value Rank
 * ^^^^^^^^^^
 *
 * This attribute indicates whether the value attribute of the variable is an
 * array and how many dimensions the array has. It may have the following
 * values:
 *
 * - ``n >= 1``: the value is an array with the specified number of dimensions
 * - ``n =  0``: the value is an array with one or more dimensions
 * - ``n = -1``: the value is a scalar
 * - ``n = -2``: the value can be a scalar or an array with any number of dimensions
 * - ``n = -3``: the value can be a scalar or a one dimensional array
 *
 * Consistency between the value rank attribute in the variable and its
 * :ref:`variabletypenode` is ensured.
 *
 * Array Dimensions
 * ^^^^^^^^^^^^^^^^
 *
 * If the value rank permits the value to be a (multi-dimensional) array, the
 * exact length in each dimensions can be further constrained with this
 * attribute.
 *
 * - For positive lengths, the variable value is guaranteed to be of the same
 *   length in this dimension.
 * - The dimension length zero is a wildcard and the actual value may have any
 *   length in this dimension.
 *
 * Consistency between the array dimensions attribute in the variable and its
 * :ref:`variabletypenode` is ensured. */

/* Indicates whether a variable contains data inline or whether it points to an
 * external data source */
typedef enum {
    UA_VALUESOURCE_DATA,
    UA_VALUESOURCE_DATASOURCE
} UA_ValueSource;

#define UA_NODE_VARIABLEATTRIBUTES                                      \
    /* Constraints on possible values */                                \
    UA_NodeId dataType;                                                 \
    UA_Int32 valueRank;                                                 \
    size_t arrayDimensionsSize;                                         \
    UA_UInt32 *arrayDimensions;                                         \
                                                                        \
    /* The current value */                                             \
    UA_ValueSource valueSource;                                         \
    union {                                                             \
        struct {                                                        \
            UA_DataValue value;                                         \
            UA_ValueCallback callback;                                  \
        } data;                                                         \
        UA_DataSource dataSource;                                       \
    } value;

typedef struct {
    UA_NODE_BASEATTRIBUTES
    UA_NODE_VARIABLEATTRIBUTES
    UA_Byte accessLevel;
    UA_Double minimumSamplingInterval;
    UA_Boolean historizing;
} UA_VariableNode;

/**
 * .. _variabletypenode:
 *
 * VariableTypeNode
 * ----------------
 *
 * VariableTypes are used to provide type definitions for variables.
 * VariableTypes constrain the data type, value rank and array dimensions
 * attributes of variable instances. Furthermore, instantiating from a specific
 * variable type may provide semantic information. For example, an instance from
 * ``MotorTemperatureVariableType`` is more meaningful than a float variable
 * instantiated from ``BaseDataVariable``. */

typedef struct {
    UA_NODE_BASEATTRIBUTES
    UA_NODE_VARIABLEATTRIBUTES
    UA_Boolean isAbstract;

    /* Members specific to open62541 */
    UA_NodeTypeLifecycle lifecycle;
} UA_VariableTypeNode;

/**
 * .. _methodnode:
 *
 * MethodNode
 * ----------
 *
 * Methods define callable functions and are invoked using the :ref:`Call
 * <method-services>` service. MethodNodes may have special properties (variable
 * childen with a ``hasProperty`` reference) with the :ref:`qualifiedname` ``(0,
 * "InputArguments")`` and ``(0, "OutputArguments")``. The input and output
 * arguments are both described via an array of ``UA_Argument``. While the Call
 * service uses a generic array of :ref:`variant` for input and output, the
 * actual argument values are checked to match the signature of the MethodNode.
 *
 * Note that the same MethodNode may be referenced from several objects (and
 * object types). For this, the NodeId of the method *and of the object
 * providing context* is part of a Call request message. */

typedef struct {
    UA_NODE_BASEATTRIBUTES
    UA_Boolean executable;

    /* Members specific to open62541 */
    UA_MethodCallback method;
} UA_MethodNode;


/** Attributes for nodes which are capable of generating events */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
/* Store active monitoredItems on this node */
# define UA_EVENT_ATTRIBUTES                                         \
    struct UA_MonitoredItem *monitoredItemQueue;
#endif

/**
 * ObjectNode
 * ----------
 *
 * Objects are used to represent systems, system components, real-world objects
 * and software objects. Objects are instances of an :ref:`object
 * type<objecttypenode>` and may contain variables, methods and further
 * objects. */

typedef struct {
    UA_NODE_BASEATTRIBUTES
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_EVENT_ATTRIBUTES
#endif
    UA_Byte eventNotifier;
} UA_ObjectNode;

/**
 * .. _objecttypenode:
 *
 * ObjectTypeNode
 * --------------
 *
 * ObjectTypes provide definitions for Objects. Abstract objects cannot be
 * instantiated. See :ref:`node-lifecycle` for the use of constructor and
 * destructor callbacks. */

typedef struct {
    UA_NODE_BASEATTRIBUTES
    UA_Boolean isAbstract;

    /* Members specific to open62541 */
    UA_NodeTypeLifecycle lifecycle;
} UA_ObjectTypeNode;

/**
 * .. _referencetypenode:
 *
 * ReferenceTypeNode
 * -----------------
 *
 * Each reference between two nodes is typed with a ReferenceType that gives
 * meaning to the relation. The OPC UA standard defines a set of ReferenceTypes
 * as a mandatory part of OPC UA information models.
 *
 * - Abstract ReferenceTypes cannot be used in actual references and are only
 *   used to structure the ReferenceTypes hierarchy
 * - Symmetric references have the same meaning from the perspective of the
 *   source and target node
 *
 * The figure below shows the hierarchy of the standard ReferenceTypes (arrows
 * indicate a ``hasSubType`` relation). Refer to Part 3 of the OPC UA
 * specification for the full semantics of each ReferenceType.
 *
 * .. graphviz::
 *
 *    digraph tree {
 *
 *    node [height=0, shape=box, fillcolor="#E5E5E5", concentrate=true]
 *
 *    references [label="References\n(Abstract, Symmetric)"]
 *    hierarchical_references [label="HierarchicalReferences\n(Abstract)"]
 *    references -> hierarchical_references
 *
 *    nonhierarchical_references [label="NonHierarchicalReferences\n(Abstract, Symmetric)"]
 *    references -> nonhierarchical_references
 *
 *    haschild [label="HasChild\n(Abstract)"]
 *    hierarchical_references -> haschild
 *
 *    aggregates [label="Aggregates\n(Abstract)"]
 *    haschild -> aggregates
 *
 *    organizes [label="Organizes"]
 *    hierarchical_references -> organizes
 *
 *    hascomponent [label="HasComponent"]
 *    aggregates -> hascomponent
 *
 *    hasorderedcomponent [label="HasOrderedComponent"]
 *    hascomponent -> hasorderedcomponent
 *
 *    hasproperty [label="HasProperty"]
 *    aggregates -> hasproperty
 *
 *    hassubtype [label="HasSubtype"]
 *    haschild -> hassubtype
 *
 *    hasmodellingrule [label="HasModellingRule"]
 *    nonhierarchical_references -> hasmodellingrule
 *
 *    hastypedefinition [label="HasTypeDefinition"]
 *    nonhierarchical_references -> hastypedefinition
 *
 *    hasencoding [label="HasEncoding"]
 *    nonhierarchical_references -> hasencoding
 *
 *    hasdescription [label="HasDescription"]
 *    nonhierarchical_references -> hasdescription
 *
 *    haseventsource [label="HasEventSource"]
 *    hierarchical_references -> haseventsource
 *
 *    hasnotifier [label="HasNotifier"]
 *    hierarchical_references -> hasnotifier
 *
 *    generatesevent [label="GeneratesEvent"]
 *    nonhierarchical_references -> generatesevent
 *
 *    alwaysgeneratesevent [label="AlwaysGeneratesEvent"]
 *    generatesevent -> alwaysgeneratesevent
 *
 *    {rank=same hierarchical_references nonhierarchical_references}
 *    {rank=same generatesevent haseventsource hasmodellingrule
 *               hasencoding hassubtype}
 *    {rank=same alwaysgeneratesevent hasproperty}
 *
 *    }
 *
 * The ReferenceType hierarchy can be extended with user-defined ReferenceTypes.
 * Many Companion Specifications for OPC UA define new ReferenceTypes to be used
 * in their domain of interest.
 *
 * For the following example of custom ReferenceTypes, we attempt to model the
 * structure of a technical system. For this, we introduce two custom
 * ReferenceTypes. First, the hierarchical ``contains`` ReferenceType indicates
 * that a system (represented by an OPC UA object) contains a component (or
 * subsystem). This gives rise to a tree-structure of containment relations. For
 * example, the motor (object) is contained in the car and the crankshaft is
 * contained in the motor. Second, the symmetric ``connectedTo`` ReferenceType
 * indicates that two components are connected. For example, the motor's
 * crankshaft is connected to the gear box. Connections are independent of the
 * containment hierarchy and can induce a general graph-structure. Further
 * subtypes of ``connectedTo`` could be used to differentiate between physical,
 * electrical and information related connections. A client can then learn the
 * layout of a (physical) system represented in an OPC UA information model
 * based on a common understanding of just two custom reference types. */

typedef struct {
    UA_NODE_BASEATTRIBUTES
    UA_Boolean isAbstract;
    UA_Boolean symmetric;
    UA_LocalizedText inverseName;
} UA_ReferenceTypeNode;

/**
 * .. _datatypenode:
 *
 * DataTypeNode
 * ------------
 *
 * DataTypes represent simple and structured data types. DataTypes may contain
 * arrays. But they always describe the structure of a single instance. In
 * open62541, DataTypeNodes in the information model hierarchy are matched to
 * ``UA_DataType`` type descriptions for :ref:`generic-types` via their NodeId.
 *
 * Abstract DataTypes (e.g. ``Number``) cannot be the type of actual values.
 * They are used to constrain values to possible child DataTypes (e.g.
 * ``UInt32``). */

typedef struct {
    UA_NODE_BASEATTRIBUTES
    UA_Boolean isAbstract;
} UA_DataTypeNode;

/**
 * ViewNode
 * --------
 *
 * Each View defines a subset of the Nodes in the AddressSpace. Views can be
 * used when browsing an information model to focus on a subset of nodes and
 * references only. ViewNodes can be created and be interacted with. But their
 * use in the :ref:`Browse<view-services>` service is currently unsupported in
 * open62541. */

typedef struct {
    UA_NODE_BASEATTRIBUTES
    UA_Byte eventNotifier;
    UA_Boolean containsNoLoops;
} UA_ViewNode;

/**
 * Nodestore Plugin API
 * ====================
 *
 * The following definitions are used for implementing custom node storage
 * backends. **Most users will want to use the default nodestore and don't need
 * to work with the nodestore API**.
 *
 * Outside of custom nodestore implementations, users should not manually edit
 * nodes. Please use the OPC UA services for that. Otherwise, all consistency
 * checks are omitted. This can crash the application eventually. */

/* For non-multithreaded access, some nodestores allow that nodes are edited
 * without a copy/replace. This is not possible when the node is only an
 * intermediate representation and stored e.g. in a database backend. */
extern const UA_Boolean inPlaceEditAllowed;

/* Nodestore context and lifecycle */
UA_StatusCode UA_Nodestore_new(void **nsCtx);
void UA_Nodestore_delete(void *nsCtx);

/**
 * The following definitions are used to create empty nodes of the different
 * node types. The memory is managed by the nodestore. Therefore, the node has
 * to be removed via a special deleteNode function. (If the new node is not
 * added to the nodestore.) */

UA_Node *
UA_Nodestore_newNode(void *nsCtx, UA_NodeClass nodeClass);

void
UA_Nodestore_deleteNode(void *nsCtx, UA_Node *node);

/**
 *``Get`` returns a pointer to an immutable node. ``Release`` indicates that the
 * pointer is no longer accessed afterwards. */

const UA_Node *
UA_Nodestore_getNode(void *nsCtx, const UA_NodeId *nodeId);

void
UA_Nodestore_releaseNode(void *nsCtx, const UA_Node *node);

/* Returns an editable copy of a node (needs to be deleted with the
 * deleteNode function or inserted / replaced into the nodestore). */
UA_StatusCode
UA_Nodestore_getNodeCopy(void *nsCtx, const UA_NodeId *nodeId,
                         UA_Node **outNode);

/* Inserts a new node into the nodestore. If the NodeId is zero, then a fresh
 * numeric NodeId is assigned. If insertion fails, the node is deleted. */
UA_StatusCode
UA_Nodestore_insertNode(void *nsCtx, UA_Node *node, UA_NodeId *addedNodeId);

/* To replace a node, get an editable copy of the node, edit and replace with
 * this function. If the node was already replaced since the copy was made,
 * UA_STATUSCODE_BADINTERNALERROR is returned. If the NodeId is not found,
 * UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases, the editable
 * node is deleted. */
UA_StatusCode
UA_Nodestore_replaceNode(void *nsCtx, UA_Node *node);

/* Removes a node from the nodestore. */
UA_StatusCode
UA_Nodestore_removeNode(void *nsCtx, const UA_NodeId *nodeId);

/* Execute a callback for every node in the nodestore. */
typedef void (*UA_NodestoreVisitor)(void *visitorCtx, const UA_Node *node);
void
UA_Nodestore_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
                     void *visitorCtx);

/**
 * Node Handling
 * =============
 *
 * To be used only in the nodestore and internally in the SDK. The following
 * methods specialize internally for the different node classes, distinguished
 * by the NodeClass attribute. */

/* Attributes must be of a matching type (VariableAttributes, ObjectAttributes,
 * and so on). The attributes are copied. Note that the attributes structs do
 * not contain NodeId, NodeClass and BrowseName. The NodeClass of the node needs
 * to be correctly set before calling this method. UA_Node_deleteMembers is
 * called on the node when an error occurs internally. */
UA_StatusCode UA_EXPORT
UA_Node_setAttributes(UA_Node *node, const void *attributes,
                      const UA_DataType *attributeType);

/* Reset the destination node and copy the content of the source */
UA_StatusCode UA_EXPORT
UA_Node_copy(const UA_Node *src, UA_Node *dst);

/* Allocate new node and copy the values from src */
UA_EXPORT UA_Node *
UA_Node_copy_alloc(const UA_Node *src);

/* Add a single reference to the node */
UA_StatusCode UA_EXPORT
UA_Node_addReference(UA_Node *node, const UA_AddReferencesItem *item);

/* Delete a single reference from the node */
UA_StatusCode UA_EXPORT
UA_Node_deleteReference(UA_Node *node, const UA_DeleteReferencesItem *item);

/* Delete all references of the node */
void UA_EXPORT
UA_Node_deleteReferences(UA_Node *node);

/* Remove all malloc'ed members of the node */
void UA_EXPORT
UA_Node_deleteMembers(UA_Node *node);

_UA_END_DECLS

#endif /* UA_SERVER_NODES_H_ */
