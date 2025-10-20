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

#include <open62541/server.h>

_UA_BEGIN_DECLS

/* Forward declaration */
#ifdef UA_ENABLE_SUBSCRIPTIONS
struct UA_MonitoredItem;
typedef struct UA_MonitoredItem UA_MonitoredItem;
#endif

/**
 * Nodestore Plugin API
 * ====================
 * **Warning!!** The structures defined in this section are only relevant for
 * the developers of custom Nodestores. The interaction with the information
 * model is possible only via the OPC UA :ref:`services`. So the following
 * sections are mainly for users that seek to understand the underlying
 * representation -- which is not directly accessible.
 *
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

typedef union {
    struct {
        UA_DataValue value;
        UA_ValueSourceNotifications notifications;
    } internal;
    struct {
        UA_DataValue **value; /* double-pointer */
        UA_ValueSourceNotifications notifications;
    } external;
    UA_CallbackValueSource callback;
} UA_ValueSourceKind;

typedef struct UA_ValueSource {
  UA_ValueSourceType type;
  UA_ValueSourceKind source;
} UA_ValueSource;

/**
 * VariableNode
 * ------------ */

#define UA_NODE_VARIABLEATTRIBUTES                                      \
    /* Constraints on possible values */                                \
    UA_NodeId dataType;                                                 \
    UA_Int32 valueRank;                                                 \
    size_t arrayDimensionsSize;                                         \
    UA_UInt32 *arrayDimensions;                                         \
                                                                        \
    /* The current value */                                             \
    UA_ValueSource valueSource;                                         \

typedef struct {
    UA_NodeHead head;
    struct {
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
    } attr;
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

typedef struct {
    UA_NodeHead head;
    UA_Boolean executable;

    /* Members specific to open62541 */
    UA_MethodCallback method;
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
 * The following structurere defines the interaction between the server and
 * Nodestore backends. */

typedef void (*UA_NodestoreVisitor)(void *visitorCtx, const UA_Node *node);

struct UA_Nodestore {
    /* Nodestore context and lifecycle */
    void (*free)(UA_Nodestore *ns);

    /* The following definitions are used to create empty nodes of the different
     * node types. The memory is managed by the nodestore. Therefore, the node
     * has to be removed via a special deleteNode function. (If the new node is
     * not added to the nodestore.) */
    UA_Node * (*newNode)(UA_Nodestore *ns, UA_NodeClass nodeClass);

    void (*deleteNode)(UA_Nodestore *ns, UA_Node *node);

    /* _getNode returns a pointer to an immutable node. Call _releaseNode to
     * indicate when the pointer is no longer accessed.
     *
     * It can be indicated if only a subset of the attributes and referencs need
     * to be accessed. That is relevant when the nodestore accesses a slow
     * storage backend for the attributes. The attribute mask is a bitfield with
     * ORed entries from UA_NodeAttributesMask. If the attributes mask is empty,
     * then only the non-standard entries (context-pointer, callbacks, etc.) are
     * returned.
     *
     * The returned node always contains the context-pointer and other fields
     * specific to open626541 (not official attributes).
     *
     * The NodeStore does not complain if attributes and references that don't
     * exist (for that node) are requested. Attributes and references in
     * addition to those specified can be returned. For example, if the full
     * node already is kept in memory by the Nodestore. */
    const UA_Node * (*getNode)(UA_Nodestore *ns, const UA_NodeId *nodeId,
                               UA_UInt32 attributeMask,
                               UA_ReferenceTypeSet references,
                               UA_BrowseDirection referenceDirections);

    /* Similar to the normal _getNode. But it can take advantage of the
     * NodePointer structure, e.g. if it contains a direct pointer. */
    const UA_Node * (*getNodeFromPtr)(UA_Nodestore *ns, UA_NodePointer ptr,
                                      UA_UInt32 attributeMask,
                                      UA_ReferenceTypeSet references,
                                      UA_BrowseDirection referenceDirections);

    /* _getEditNode returns a pointer to a mutable version of the node. A
     * plugin implementation that keeps all nodes in RAM can return the same
     * pointer from _getNode and _getEditNode. The differences are more
     * important if, for example, nodes are stored in a backend database. Then
     * the _getEditNode version is used to indicate that modifications are
     * being made.
     *
     * Call _releaseNode to indicate when editing is done and the pointer is
     * no longer used. Note that changes are not (necessarily) visible in other
     * (const) node-pointers that were previously retrieved. Changes are however
     * visible in all newly retrieved node-pointers for the given NodeId after
     * calling _releaseNode.
     *
     * The attribute-mask and reference-description indicate if only a subset of
     * the attributes and referencs are to be modified. Other attributes and
     * references shall not be changed. */
    UA_Node * (*getEditNode)(UA_Nodestore *ns, const UA_NodeId *nodeId,
                             UA_UInt32 attributeMask,
                             UA_ReferenceTypeSet references,
                             UA_BrowseDirection referenceDirections);

    /* Similar to _getEditNode. But it can take advantage of the NodePointer
     * structure, e.g. if it contains a direct pointer. */
    UA_Node * (*getEditNodeFromPtr)(UA_Nodestore *ns, UA_NodePointer ptr,
                                    UA_UInt32 attributeMask,
                                    UA_ReferenceTypeSet references,
                                    UA_BrowseDirection referenceDirections);

    /* Release a node that has been retrieved with ``getNode`` or
     * ``getNodeFromPtr``. */
    void (*releaseNode)(UA_Nodestore *ns, const UA_Node *node);

    /* Returns an editable copy of a node (needs to be deleted with the
     * deleteNode function or inserted / replaced into the nodestore). */
    UA_StatusCode (*getNodeCopy)(UA_Nodestore *ns, const UA_NodeId *nodeId,
                                 UA_Node **outNode);

    /* Inserts a new node into the nodestore. If the NodeId is zero, then a
     * fresh numeric NodeId is assigned. If insertion fails, the node is
     * deleted. */
    UA_StatusCode (*insertNode)(UA_Nodestore *ns, UA_Node *node,
                                UA_NodeId *addedNodeId);

    /* To replace a node, get an editable copy of the node, edit and replace
     * with this function. If the node was already replaced since the copy was
     * made, UA_STATUSCODE_BADINTERNALERROR is returned. If the NodeId is not
     * found, UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases,
     * the editable node is deleted. */
    UA_StatusCode (*replaceNode)(UA_Nodestore *ns, UA_Node *node);

    /* Removes a node from the nodestore. */
    UA_StatusCode (*removeNode)(UA_Nodestore *ns, const UA_NodeId *nodeId);

    /* Maps the ReferenceTypeIndex used for the references to the NodeId of the
     * ReferenceType. The returned pointer is stable until the Nodestore is
     * deleted. */
    const UA_NodeId * (*getReferenceTypeId)(UA_Nodestore *ns,
                                            UA_Byte refTypeIndex);

    /* Execute a callback for every node in the nodestore. */
    void (*iterate)(UA_Nodestore *ns, UA_NodestoreVisitor visitor,
                    void *visitorCtx);
};

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
