/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017, 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

/* !!! Warning !!!
 *
 * If you are not developing a nodestore plugin, then you should not work with
 * the definitions from this file directly. The underlying node structures are
 * not meant to be used directly by end users. Please use the public server API
 * / OPC UA services to interact with the information model. */

#include <open62541/util.h>

_UA_BEGIN_DECLS

/* Forward declaration */
#ifdef UA_ENABLE_SUBSCRIPTIONS
struct UA_MonitoredItem;
typedef struct UA_MonitoredItem UA_MonitoredItem;
#endif

/**
 * Node Store Plugin API
 * =====================
 *
 * **Warning!!** The structures defined in this section are only relevant for
 * the developers of custom Nodestores. The interaction with the information
 * model is possible only via the OPC UA :ref:`services`. So the following
 * sections are purely informational so that users may have a clear mental
 * model of the underlying representation.
 *
 * .. _node-lifecycle:
 *
 * Node Lifecycle: Constructors, Destructors and Node Contexts
 * -----------------------------------------------------------
 *
 * To finalize the instantiation of a node, a (user-defined) constructor
 * callback is executed. There can be both a global constructor for all nodes
 * and node-type constructor specific to the TypeDefinition of the new node
 * (attached to an ObjectTypeNode or VariableTypeNode).
 *
 * In the hierarchy of ObjectTypes and VariableTypes, only the constructor of
 * the (lowest) type defined for the new node is executed. Note that every
 * Object and Variable can have only one ``isTypeOf`` reference. But type-nodes
 * can technically have several ``hasSubType`` references to implement multiple
 * inheritance. Issues of (multiple) inheritance in the constructor need to be
 * solved by the user.
 *
 * When a node is destroyed, the node-type destructor is called before the
 * global destructor. So the overall node lifecycle is as follows:
 *
 * 1. Global Constructor (set in the server config)
 * 2. Node-Type Constructor (for VariableType or ObjectTypes)
 * 3. (Usage-period of the Node)
 * 4. Node-Type Destructor
 * 5. Global Destructor
 *
 * The constructor and destructor callbacks can be set to ``NULL`` and are not
 * used in that case. If the node-type constructor fails, the global destructor
 * will be called before removing the node. The destructors are assumed to never
 * fail.
 *
 * Every node carries a user-context and a constructor-context pointer. The
 * user-context is used to attach custom data to a node. But the (user-defined)
 * constructors and destructors may replace the user-context pointer if they
 * wish to do so. The initial value for the constructor-context is ``NULL``.
 * When the ``AddNodes`` service is used over the network, the user-context
 * pointer of the new node is also initially set to ``NULL``.
 *
 * Global Node Lifecycle
 * ~~~~~~~~~~~~~~~~~~~~~~
 * Global constructor and destructor callbacks used for every node type.
 * To be set in the server config.
 */

typedef struct {
    /* Can be NULL. May replace the nodeContext */
    UA_StatusCode (*constructor)(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *nodeId, void **nodeContext);

    /* Can be NULL. The context cannot be replaced since the node is destroyed
     * immediately afterwards anyway. */
    void (*destructor)(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *nodeId, void *nodeContext);

    /* Can be NULL. Called during recursive node instantiation. While mandatory
     * child nodes are automatically created if not already present, optional child
     * nodes are not. This callback can be used to define whether an optional child
     * node should be created.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param sourceNodeId Source node from the type definition. If the new node
     *        shall be created, it will be a copy of this node.
     * @param targetParentNodeId Parent of the potential new child node
     * @param referenceTypeId Identifies the reference type which that the parent
     *        node has to the new node.
     * @return Return UA_TRUE if the child node shall be instantiated,
     *         UA_FALSE otherwise. */
    UA_Boolean (*createOptionalChild)(UA_Server *server,
                                      const UA_NodeId *sessionId,
                                      void *sessionContext,
                                      const UA_NodeId *sourceNodeId,
                                      const UA_NodeId *targetParentNodeId,
                                      const UA_NodeId *referenceTypeId);

    /* Can be NULL. Called when a node is to be copied during recursive
     * node instantiation. Allows definition of the NodeId for the new node.
     * If the callback is set to NULL or the resulting NodeId is UA_NODEID_NUMERIC(X,0)
     * an unused nodeid in namespace X will be used. E.g. passing UA_NODEID_NULL will
     * result in a NodeId in namespace 0.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param sourceNodeId Source node of the copy operation
     * @param targetParentNodeId Parent node of the new node
     * @param referenceTypeId Identifies the reference type which that the parent
     *        node has to the new node. */
    UA_StatusCode (*generateChildNodeId)(UA_Server *server,
                                         const UA_NodeId *sessionId, void *sessionContext,
                                         const UA_NodeId *sourceNodeId,
                                         const UA_NodeId *targetParentNodeId,
                                         const UA_NodeId *referenceTypeId,
                                         UA_NodeId *targetNodeId);
} UA_GlobalNodeLifecycle;

/**
 * Node Type Lifecycle
 * ~~~~~~~~~~~~~~~~~~~
 * Constructor and destructors for specific object and variable types. */
typedef struct {
    /* Can be NULL. May replace the nodeContext */
    UA_StatusCode (*constructor)(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *typeNodeId, void *typeNodeContext,
                                 const UA_NodeId *nodeId, void **nodeContext);

    /* Can be NULL. May replace the nodeContext. */
    void (*destructor)(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *typeNodeId, void *typeNodeContext,
                       const UA_NodeId *nodeId, void **nodeContext);
} UA_NodeTypeLifecycle;

/**
 * ReferenceType Bitfield Representation
 * -------------------------------------
 * ReferenceTypes have an alternative represention as an index into a bitfield
 * for fast comparison. The index is generated when the corresponding
 * ReferenceTypeNode is added. By bounding the number of ReferenceTypes that can
 * exist in the server, the bitfield can represent a set of an combination of
 * ReferenceTypes.
 *
 * Every ReferenceTypeNode contains a bitfield with the set of all its subtypes.
 * This speeds up the Browse services substantially.
 *
 * The following ReferenceTypes have a fixed index. The NS0 bootstrapping
 * creates these ReferenceTypes in-order. */
#define UA_REFERENCETYPEINDEX_REFERENCES 0
#define UA_REFERENCETYPEINDEX_HASSUBTYPE 1
#define UA_REFERENCETYPEINDEX_AGGREGATES 2
#define UA_REFERENCETYPEINDEX_HIERARCHICALREFERENCES 3
#define UA_REFERENCETYPEINDEX_NONHIERARCHICALREFERENCES 4
#define UA_REFERENCETYPEINDEX_HASCHILD 5
#define UA_REFERENCETYPEINDEX_ORGANIZES 6
#define UA_REFERENCETYPEINDEX_HASEVENTSOURCE 7
#define UA_REFERENCETYPEINDEX_HASMODELLINGRULE 8
#define UA_REFERENCETYPEINDEX_HASENCODING 9
#define UA_REFERENCETYPEINDEX_HASDESCRIPTION 10
#define UA_REFERENCETYPEINDEX_HASTYPEDEFINITION 11
#define UA_REFERENCETYPEINDEX_GENERATESEVENT 12
#define UA_REFERENCETYPEINDEX_HASPROPERTY 13
#define UA_REFERENCETYPEINDEX_HASCOMPONENT 14
#define UA_REFERENCETYPEINDEX_HASNOTIFIER 15
#define UA_REFERENCETYPEINDEX_HASORDEREDCOMPONENT 16
#define UA_REFERENCETYPEINDEX_HASINTERFACE 17

/* The maximum number of ReferrenceTypes. Must be a multiple of 32. */
#define UA_REFERENCETYPESET_MAX 128
typedef struct {
    UA_UInt32 bits[UA_REFERENCETYPESET_MAX / 32];
} UA_ReferenceTypeSet;

UA_EXPORT extern const UA_ReferenceTypeSet UA_REFERENCETYPESET_NONE;
UA_EXPORT extern const UA_ReferenceTypeSet UA_REFERENCETYPESET_ALL;

static UA_INLINE void
UA_ReferenceTypeSet_init(UA_ReferenceTypeSet *set) {
    memset(set, 0, sizeof(UA_ReferenceTypeSet));
}

static UA_INLINE UA_ReferenceTypeSet
UA_REFTYPESET(UA_Byte index) {
    UA_Byte i = index / 32, j = index % 32;
    UA_ReferenceTypeSet set;
    UA_ReferenceTypeSet_init(&set);
    set.bits[i] |= ((UA_UInt32)1) << j;
    return set;
}

static UA_INLINE UA_ReferenceTypeSet
UA_ReferenceTypeSet_union(const UA_ReferenceTypeSet setA,
                          const UA_ReferenceTypeSet setB) {
    UA_ReferenceTypeSet set;
    for(size_t i = 0; i < UA_REFERENCETYPESET_MAX / 32; i++)
        set.bits[i] = setA.bits[i] | setB.bits[i];
    return set;
}

static UA_INLINE UA_Boolean
UA_ReferenceTypeSet_contains(const UA_ReferenceTypeSet *set, UA_Byte index) {
    UA_Byte i = index / 32, j = index % 32;
    return !!(set->bits[i] & (((UA_UInt32)1) << j));
}

/**
 * Node Pointer
 * ------------
 *
 * The "native" format for reference between nodes is the ExpandedNodeId. That
 * is, references can also point to external servers. In practice, most
 * references point to local nodes using numerical NodeIds from the
 * standard-defined namespace zero. In order to save space (and time),
 * pointer-tagging is used for compressed "NodePointer" representations.
 * Numerical NodeIds are immediately contained in the pointer. Full NodeIds and
 * ExpandedNodeIds are behind a pointer indirection. If the Nodestore supports
 * it, a NodePointer can also be an actual pointer to the target node.
 *
 * Depending on the processor architecture, some numerical NodeIds don't fit
 * into an immediate encoding and are kept as pointers. ExpandedNodeIds may be
 * internally translated to "normal" NodeIds. Use the provided functions to
 * generate NodePointers that fit the assumptions for the local architecture. */

/* Forward declaration. All node structures begin with the NodeHead. */
struct UA_NodeHead;
typedef struct UA_NodeHead UA_NodeHead;

/* Tagged Pointer structure. */
typedef union {
    uintptr_t immediate;                 /* 00: Small numerical NodeId */
    const UA_NodeId *id;                 /* 01: Pointer to NodeId */
    const UA_ExpandedNodeId *expandedId; /* 10: Pointer to ExternalNodeId */
    const UA_NodeHead *node;             /* 11: Pointer to a node */
} UA_NodePointer;

/* Sets the pointer to an immediate NodeId "ns=0;i=0" similar to a freshly
 * initialized UA_NodeId */
static UA_INLINE void
UA_NodePointer_init(UA_NodePointer *np) { np->immediate = 0; }

/* NodeId and ExpandedNodeId targets are freed */
void UA_EXPORT
UA_NodePointer_clear(UA_NodePointer *np);

/* Makes a deep copy */
UA_StatusCode UA_EXPORT
UA_NodePointer_copy(UA_NodePointer in, UA_NodePointer *out);

/* Test if an ExpandedNodeId or a local NodeId */
UA_Boolean UA_EXPORT
UA_NodePointer_isLocal(UA_NodePointer np);

UA_Order UA_EXPORT
UA_NodePointer_order(UA_NodePointer p1, UA_NodePointer p2);

static UA_INLINE UA_Boolean
UA_NodePointer_equal(UA_NodePointer p1, UA_NodePointer p2) {
    return (UA_NodePointer_order(p1, p2) == UA_ORDER_EQ);
}

/* Cannot fail. The resulting NodePointer can point to the memory from the
 * NodeId. Make a deep copy if required. */
UA_NodePointer UA_EXPORT
UA_NodePointer_fromNodeId(const UA_NodeId *id);

/* Cannot fail. The resulting NodePointer can point to the memory from the
 * ExpandedNodeId. Make a deep copy if required. */
UA_NodePointer UA_EXPORT
UA_NodePointer_fromExpandedNodeId(const UA_ExpandedNodeId *id);

/* Can point to the memory from the NodePointer */
UA_ExpandedNodeId UA_EXPORT
UA_NodePointer_toExpandedNodeId(UA_NodePointer np);

/* Can point to the memory from the NodePointer. Discards the ServerIndex and
 * NamespaceUri of a potential ExpandedNodeId inside the NodePointer. Test
 * before if the NodePointer is local. */
UA_NodeId UA_EXPORT
UA_NodePointer_toNodeId(UA_NodePointer np);

/**
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

typedef struct {
    UA_NodePointer targetId;  /* Has to be the first entry */
    UA_UInt32 targetNameHash; /* Hash of the target's BrowseName. Set to zero
                               * if the target is remote. */
} UA_ReferenceTarget;

typedef struct UA_ReferenceTargetTreeElem {
    UA_ReferenceTarget target;   /* Has to be the first entry */
    UA_UInt32 targetIdHash;      /* Hash of the targetId */
    struct {
        struct UA_ReferenceTargetTreeElem *left;
        struct UA_ReferenceTargetTreeElem *right;
    } idTreeEntry;
    struct {
        struct UA_ReferenceTargetTreeElem *left;
        struct UA_ReferenceTargetTreeElem *right;
    } nameTreeEntry;
} UA_ReferenceTargetTreeElem;


/* List of reference targets with the same reference type and direction. Uses
 * either an array or a tree structure. The SDK will not change the type of
 * reference target structure internally. The nodestore implementations may
 * switch internally when a node is updated.
 *
 * The recommendation is to switch to a tree once the number of refs > 8. */
typedef struct {
    union {
        /* Organize the references in an array. Uses less memory, but incurs
         * lookups in linear time. Recommended if the number of references is
         * known to be small. */
        UA_ReferenceTarget *array;

        /* Organize the references in a tree for fast lookup. Use
         * UA_Node_addReference and UA_Node_deleteReference to modify the
         * tree-structure. The binary tree implementation (and absolute ordering
         * / duplicate browseNames are allowed) are not exposed otherwise in the
         * public API. */
        struct {
            UA_ReferenceTargetTreeElem *idRoot;   /* Lookup based on target id */
            UA_ReferenceTargetTreeElem *nameRoot; /* Lookup based on browseName*/
        } tree;
    } targets;
    size_t targetsSize;
    UA_Boolean hasRefTree; /* RefTree or RefArray? */
    UA_Byte referenceTypeIndex;
    UA_Boolean isInverse;
} UA_NodeReferenceKind;

/* Iterate over the references. Aborts when the first callback return a non-NULL
 * pointer and returns that pointer. Do not modify the reference targets during
 * the iteration. */
typedef void *
(*UA_NodeReferenceKind_iterateCallback)(void *context, UA_ReferenceTarget *target);

UA_EXPORT void *
UA_NodeReferenceKind_iterate(UA_NodeReferenceKind *rk,
                             UA_NodeReferenceKind_iterateCallback callback,
                             void *context);

/* Returns the entry for the targetId or NULL if not found */
UA_EXPORT const UA_ReferenceTarget *
UA_NodeReferenceKind_findTarget(const UA_NodeReferenceKind *rk,
                                const UA_ExpandedNodeId *targetId);

/* Switch between array and tree representation. Does nothing upon error (e.g.
 * out-of-memory). */
UA_EXPORT UA_StatusCode
UA_NodeReferenceKind_switch(UA_NodeReferenceKind *rk);

/* Singly-linked LocalizedText list */
typedef struct UA_LocalizedTextListEntry {
    struct UA_LocalizedTextListEntry *next;
    UA_LocalizedText localizedText;
} UA_LocalizedTextListEntry;

/* Every Node starts with these attributes */
struct UA_NodeHead {
    UA_NodeId nodeId;
    UA_NodeClass nodeClass;
    UA_QualifiedName browseName;

    /* A node can have different localizations for displayName and description.
     * The server selects a suitable localization depending on the locale ids
     * that are set for the current session.
     *
     * Locales are added simply by writing a LocalizedText value with a new
     * locale. A locale can be removed by writing a LocalizedText value of the
     * corresponding locale with an empty text field. */
    UA_LocalizedTextListEntry *displayName;
    UA_LocalizedTextListEntry *description;

    UA_UInt32 writeMask;
    size_t referencesSize;
    UA_NodeReferenceKind *references;

    /* Members specific to open62541 */
    void *context;
    UA_Boolean constructed; /* Constructors were called */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_MonitoredItem *monitoredItems; /* MonitoredItems for Events and immediate
                                       * DataChanges (no sampling interval). */
#endif
};

/**
 * VariableNode
 * ------------ */

/* Indicates whether a variable contains data inline or whether it points to an
 * external data source */
typedef enum {
    UA_VALUESOURCE_DATA,
    UA_VALUESOURCE_DATASOURCE
} UA_ValueSource;

typedef struct {
    /* Called before the value attribute is read. It is possible to write into the
     * value attribute during onRead (using the write service). The node is
     * re-opened afterwards so that changes are considered in the following read
     * operation.
     *
     * @param handle Points to user-provided data for the callback.
     * @param nodeid The identifier of the node.
     * @param data Points to the current node value.
     * @param range Points to the numeric range the client wants to read from
     *        (or NULL). */
    void (*onRead)(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *nodeid,
                   void *nodeContext, const UA_NumericRange *range,
                   const UA_DataValue *value);

    /* Called after writing the value attribute. The node is re-opened after
     * writing so that the new value is visible in the callback.
     *
     * @param server The server executing the callback
     * @sessionId The identifier of the session
     * @sessionContext Additional data attached to the session
     *                 in the access control layer
     * @param nodeid The identifier of the node.
     * @param nodeUserContext Additional data attached to the node by
     *        the user.
     * @param nodeConstructorContext Additional data attached to the node
     *        by the type constructor(s).
     * @param range Points to the numeric range the client wants to write to (or
     *        NULL). */
    void (*onWrite)(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *nodeId,
                    void *nodeContext, const UA_NumericRange *range,
                    const UA_DataValue *data);
} UA_ValueCallback;

typedef struct {
    /* Copies the data from the source into the provided value.
     *
     * !! ZERO-COPY OPERATIONS POSSIBLE !!
     * It is not required to return a copy of the actual content data. You can
     * return a pointer to memory owned by the user. Memory can be reused
     * between read callbacks of a DataSource, as the result is already encoded
     * on the network buffer between each read operation.
     *
     * To use zero-copy reads, set the value of the `value->value` Variant
     * without copying, e.g. with `UA_Variant_setScalar`. Then, also set
     * `value->value.storageType` to `UA_VARIANT_DATA_NODELETE` to prevent the
     * memory being cleaned up. Don't forget to also set `value->hasValue` to
     * true to indicate the presence of a value.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param nodeId The identifier of the node being read from
     * @param nodeContext Additional data attached to the node by the user
     * @param includeSourceTimeStamp If true, then the datasource is expected to
     *        set the source timestamp in the returned value
     * @param range If not null, then the datasource shall return only a
     *        selection of the (nonscalar) data. Set
     *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not
     *        apply
     * @param value The (non-null) DataValue that is returned to the client. The
     *        data source sets the read data, the result status and optionally a
     *        sourcetimestamp.
     * @return Returns a status code for logging. Error codes intended for the
     *         original caller are set in the value. If an error is returned,
     *         then no releasing of the value is done
     */
    UA_StatusCode (*read)(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, UA_Boolean includeSourceTimeStamp,
                          const UA_NumericRange *range, UA_DataValue *value);

    /* Write into a data source. This method pointer can be NULL if the
     * operation is unsupported.
     *
     * @param server The server executing the callback
     * @param sessionId The identifier of the session
     * @param sessionContext Additional data attached to the session in the
     *        access control layer
     * @param nodeId The identifier of the node being written to
     * @param nodeContext Additional data attached to the node by the user
     * @param range If not NULL, then the datasource shall return only a
     *        selection of the (nonscalar) data. Set
     *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not
     *        apply
     * @param value The (non-NULL) DataValue that has been written by the client.
     *        The data source contains the written data, the result status and
     *        optionally a sourcetimestamp
     * @return Returns a status code for logging. Error codes intended for the
     *         original caller are set in the value. If an error is returned,
     *         then no releasing of the value is done
     */
    UA_StatusCode (*write)(UA_Server *server, const UA_NodeId *sessionId,
                           void *sessionContext, const UA_NodeId *nodeId,
                           void *nodeContext, const UA_NumericRange *range,
                           const UA_DataValue *value);
} UA_DataSource;

/**
 * .. _value-callback:
 *
 * Value Callback
 * ~~~~~~~~~~~~~~
 * Value Callbacks can be attached to variable and variable type nodes. If
 * not ``NULL``, they are called before reading and after writing respectively. */
typedef struct {
    /* Called before the value attribute is read. The external value source can be
     * be updated and/or locked during this notification call. After this function returns
     * to the core, the external value source is readed immediately.
    */
    UA_StatusCode (*notificationRead)(UA_Server *server, const UA_NodeId *sessionId,
                                      void *sessionContext, const UA_NodeId *nodeid,
                                      void *nodeContext, const UA_NumericRange *range);

    /* Called after writing the value attribute. The node is re-opened after
     * writing so that the new value is visible in the callback.
     *
     * @param server The server executing the callback
     * @sessionId The identifier of the session
     * @sessionContext Additional data attached to the session
     *                 in the access control layer
     * @param nodeid The identifier of the node.
     * @param nodeUserContext Additional data attached to the node by
     *        the user.
     * @param nodeConstructorContext Additional data attached to the node
     *        by the type constructor(s).
     * @param range Points to the numeric range the client wants to write to (or
     *        NULL). */
    UA_StatusCode (*userWrite)(UA_Server *server, const UA_NodeId *sessionId,
                               void *sessionContext, const UA_NodeId *nodeId,
                               void *nodeContext, const UA_NumericRange *range,
                               const UA_DataValue *data);
} UA_ExternalValueCallback;

typedef enum {
    UA_VALUEBACKENDTYPE_NONE,
    UA_VALUEBACKENDTYPE_INTERNAL,
    UA_VALUEBACKENDTYPE_DATA_SOURCE_CALLBACK,
    UA_VALUEBACKENDTYPE_EXTERNAL
} UA_ValueBackendType;

typedef struct {
    UA_ValueBackendType backendType;
    union {
        struct {
            UA_DataValue value;
            UA_ValueCallback callback;
        } internal;
        UA_DataSource dataSource;
        struct {
            UA_DataValue **value;
            UA_ExternalValueCallback callback;
        } external;
    } backend;
} UA_ValueBackend;

#define UA_NODE_VARIABLEATTRIBUTES                                      \
    /* Constraints on possible values */                                \
    UA_NodeId dataType;                                                 \
    UA_Int32 valueRank;                                                 \
    size_t arrayDimensionsSize;                                         \
    UA_UInt32 *arrayDimensions;                                         \
                                                                        \
    UA_ValueBackend valueBackend;                                       \
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
    UA_NodeHead head;
    UA_NODE_VARIABLEATTRIBUTES
    UA_Byte accessLevel;
    UA_Double minimumSamplingInterval;
    UA_Boolean historizing;

    /* Members specific to open62541 */
    UA_Boolean isDynamic; /* Some variables are "static" in the sense that they
                           * are not attached to a dynamic process in the
                           * background. Only dynamic variables conserve source
                           * and server timestamp for the value attribute.
                           * Static variables have timestamps of "now". */
} UA_VariableNode;

/**
 * VariableTypeNode
 * ---------------- */

typedef struct {
    UA_NodeHead head;
    UA_NODE_VARIABLEATTRIBUTES
    UA_Boolean isAbstract;

    /* Members specific to open62541 */
    UA_NodeTypeLifecycle lifecycle;
} UA_VariableTypeNode;

/**
 * MethodNode
 * ---------- */

typedef UA_StatusCode
(*UA_MethodCallback)(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize,
                     const UA_Variant *input, size_t outputSize,
                     UA_Variant *output);

typedef struct {
    UA_NodeHead head;
    UA_Boolean executable;

    /* Members specific to open62541 */
    UA_MethodCallback method;
#if UA_MULTITHREADING >= 100
    UA_Boolean async; /* Indicates an async method call */
#endif
} UA_MethodNode;

/**
 * ObjectNode
 * ---------- */

typedef struct {
    UA_NodeHead head;
    UA_Byte eventNotifier;
} UA_ObjectNode;

/**
 * ObjectTypeNode
 * -------------- */

typedef struct {
    UA_NodeHead head;
    UA_Boolean isAbstract;

    /* Members specific to open62541 */
    UA_NodeTypeLifecycle lifecycle;
} UA_ObjectTypeNode;

/**
 * ReferenceTypeNode
 * ----------------- */

typedef struct {
    UA_NodeHead head;
    UA_Boolean isAbstract;
    UA_Boolean symmetric;
    UA_LocalizedText inverseName;

    /* Members specific to open62541 */
    UA_Byte referenceTypeIndex;
    UA_ReferenceTypeSet subTypes; /* contains the type itself as well */
} UA_ReferenceTypeNode;

/**
 * DataTypeNode
 * ------------ */

typedef struct {
    UA_NodeHead head;
    UA_Boolean isAbstract;
} UA_DataTypeNode;

/**
 * ViewNode
 * -------- */

typedef struct {
    UA_NodeHead head;
    UA_Byte eventNotifier;
    UA_Boolean containsNoLoops;
} UA_ViewNode;

/**
 * Node Union
 * ----------
 *
 * A union that represents any kind of node. The node head can always be used.
 * Check the NodeClass before accessing specific content.
 */

typedef union {
    UA_NodeHead head;
    UA_VariableNode variableNode;
    UA_VariableTypeNode variableTypeNode;
    UA_MethodNode methodNode;
    UA_ObjectNode objectNode;
    UA_ObjectTypeNode objectTypeNode;
    UA_ReferenceTypeNode referenceTypeNode;
    UA_DataTypeNode dataTypeNode;
    UA_ViewNode viewNode;
} UA_Node;

/**
 * Nodestore
 * ---------
 *
 * The following definitions are used for implementing custom node storage
 * backends. **Most users will want to use the default nodestore and don't need
 * to work with the nodestore API**.
 *
 * Outside of custom nodestore implementations, users should not manually edit
 * nodes. Please use the OPC UA services for that. Otherwise, all consistency
 * checks are omitted. This can crash the application eventually. */

typedef void (*UA_NodestoreVisitor)(void *visitorCtx, const UA_Node *node);

typedef struct {
    /* Nodestore context and lifecycle */
    void *context;
    void (*clear)(void *nsCtx);

    /* The following definitions are used to create empty nodes of the different
     * node types. The memory is managed by the nodestore. Therefore, the node
     * has to be removed via a special deleteNode function. (If the new node is
     * not added to the nodestore.) */
    UA_Node * (*newNode)(void *nsCtx, UA_NodeClass nodeClass);

    void (*deleteNode)(void *nsCtx, UA_Node *node);

    /* ``Get`` returns a pointer to an immutable node. Call ``releaseNode`` to
     * indicate when the pointer is no longer accessed.
     *
     * It can be indicated if only a subset of the attributes and referencs need
     * to be accessed. That is relevant when the nodestore accesses a slow
     * storage backend for the attributes. The attribute mask is a bitfield with
     * ORed entries from UA_NodeAttributesMask.
     *
     * The returned node always contains the context-pointer and other fields
     * specific to open626541 (not official attributes).
     *
     * The NodeStore does not complain if attributes and references that don't
     * exist (for that node) are requested. Attributes and references in
     * addition to those specified can be returned. For example, if the full
     * node already is kept in memory by the Nodestore. */
    const UA_Node * (*getNode)(void *nsCtx, const UA_NodeId *nodeId,
                               UA_UInt32 attributeMask,
                               UA_ReferenceTypeSet references,
                               UA_BrowseDirection referenceDirections);

    /* Similar to the normal ``getNode``. But it can take advantage of the
     * NodePointer structure, e.g. if it contains a direct pointer. */
    const UA_Node * (*getNodeFromPtr)(void *nsCtx, UA_NodePointer ptr,
                                      UA_UInt32 attributeMask,
                                      UA_ReferenceTypeSet references,
                                      UA_BrowseDirection referenceDirections);

    /* Release a node that has been retrieved with ``getNode`` or
     * ``getNodeFromPtr``. */
    void (*releaseNode)(void *nsCtx, const UA_Node *node);

    /* Returns an editable copy of a node (needs to be deleted with the
     * deleteNode function or inserted / replaced into the nodestore). */
    UA_StatusCode (*getNodeCopy)(void *nsCtx, const UA_NodeId *nodeId,
                                 UA_Node **outNode);

    /* Inserts a new node into the nodestore. If the NodeId is zero, then a
     * fresh numeric NodeId is assigned. If insertion fails, the node is
     * deleted. */
    UA_StatusCode (*insertNode)(void *nsCtx, UA_Node *node,
                                UA_NodeId *addedNodeId);

    /* To replace a node, get an editable copy of the node, edit and replace
     * with this function. If the node was already replaced since the copy was
     * made, UA_STATUSCODE_BADINTERNALERROR is returned. If the NodeId is not
     * found, UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases,
     * the editable node is deleted. */
    UA_StatusCode (*replaceNode)(void *nsCtx, UA_Node *node);

    /* Removes a node from the nodestore. */
    UA_StatusCode (*removeNode)(void *nsCtx, const UA_NodeId *nodeId);

    /* Maps the ReferenceTypeIndex used for the references to the NodeId of the
     * ReferenceType. The returned pointer is stable until the Nodestore is
     * deleted. */
    const UA_NodeId * (*getReferenceTypeId)(void *nsCtx, UA_Byte refTypeIndex);

    /* Execute a callback for every node in the nodestore. */
    void (*iterate)(void *nsCtx, UA_NodestoreVisitor visitor,
                    void *visitorCtx);
} UA_Nodestore;

/* Attributes must be of a matching type (VariableAttributes, ObjectAttributes,
 * and so on). The attributes are copied. Note that the attributes structs do
 * not contain NodeId, NodeClass and BrowseName. The NodeClass of the node needs
 * to be correctly set before calling this method. UA_Node_clear is called on
 * the node when an error occurs internally. */
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
UA_Node_addReference(UA_Node *node, UA_Byte refTypeIndex, UA_Boolean isForward,
                     const UA_ExpandedNodeId *targetNodeId,
                     UA_UInt32 targetBrowseNameHash);

/* Delete a single reference from the node */
UA_StatusCode UA_EXPORT
UA_Node_deleteReference(UA_Node *node, UA_Byte refTypeIndex, UA_Boolean isForward,
                        const UA_ExpandedNodeId *targetNodeId);

/* Deletes references from the node which are not matching any type in the given
 * array. Could be used to e.g. delete all the references, except
 * 'HASMODELINGRULE' */
void UA_EXPORT
UA_Node_deleteReferencesSubset(UA_Node *node, const UA_ReferenceTypeSet *keepSet);

/* Delete all references of the node */
void UA_EXPORT
UA_Node_deleteReferences(UA_Node *node);

/* Remove all malloc'ed members of the node and reset */
void UA_EXPORT
UA_Node_clear(UA_Node *node);

_UA_END_DECLS

#endif /* UA_NODESTORE_H_ */
