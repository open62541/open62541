#include "adhoc_nodestore.h"
#include <open62541/plugin/nodestore.h>
#include <open62541/server.h>
#include <open62541/types.h>

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))
#endif

/***************/
/* AdHoc Nodes */
/***************/

/* The default Nodestore is simply a hash-map from NodeIds to Nodes. To find an
 * entry, iterate over candidate positions according to the NodeId hash.
 *
 * - Tombstone or non-matching NodeId: continue searching
 * - Matching NodeId: Return the entry
 * - NULL: Abort the search */

/* Mock-up Backend Node Representation
 * This represents the future backend system that will provide node information ad-hoc.
 * For now, we use a static structure to simulate this functionality. */

typedef struct {
    UA_UInt32 nodeId;              /* Numeric NodeId */
    UA_UInt16 namespaceIndex;      /* Namespace index */
    char *name;                    /* Node name */
    char *browseName;              /* Browse name */
    UA_NodeClass nodeClass;        /* UA NodeClass (Object, Variable, Method, etc.) */
    UA_UInt32 parentNodeId;        /* Parent node ID (0 for root-level nodes) */
    UA_UInt16 parentNamespaceIndex; /* Parent namespace index */
    UA_UInt32 referenceTypeId;     /* Reference type to parent (e.g., HasComponent, Organizes) */
    char *guid;                    /* GUID as string */
    char *value;                   /* String representation of value (for variables) */
} MockBackendNode;

/* Static mock backend data - represents 20 hierarchical nodes */
static const MockBackendNode mockBackendNodes[] = {
    /* Root folder under Objects */
    {1000, 1, "PilzDeviceRoot", "PilzDeviceRoot", UA_NODECLASS_OBJECT, 
     85, 0, 35, "550e8400-e29b-41d4-a716-446655440000", NULL},
    
    /* Device Information folder */
    {1001, 1, "DeviceInfo", "DeviceInfo", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440001", NULL},
    
    /* Device variables under DeviceInfo */
    {1002, 1, "DeviceName", "DeviceName", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440002", "Pilz Safety Controller"},
    
    {1003, 1, "SerialNumber", "SerialNumber", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440003", "PSC-12345-67890"},
    
    {1004, 1, "FirmwareVersion", "FirmwareVersion", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440004", "v2.1.5"},
    
    {1005, 1, "ManufacturerName", "ManufacturerName", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440005", "Pilz GmbH & Co. KG"},
    
    /* Safety System folder */
    {1010, 1, "SafetySystem", "SafetySystem", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440010", NULL},
    
    /* Safety variables */
    {1011, 1, "SafetyState", "SafetyState", UA_NODECLASS_VARIABLE, 
     1010, 1, 47, "550e8400-e29b-41d4-a716-446655440011", "SAFE"},
    
    {1012, 1, "EmergencyStopActive", "EmergencyStopActive", UA_NODECLASS_VARIABLE, 
     1010, 1, 47, "550e8400-e29b-41d4-a716-446655440012", "false"},
    
    {1013, 1, "SafetyOutputs", "SafetyOutputs", UA_NODECLASS_OBJECT, 
     1010, 1, 47, "550e8400-e29b-41d4-a716-446655440013", NULL},
    
    /* Individual safety outputs */
    {1014, 1, "Output1", "Output1", UA_NODECLASS_VARIABLE, 
     1013, 1, 47, "550e8400-e29b-41d4-a716-446655440014", "true"},
    
    {1015, 1, "Output2", "Output2", UA_NODECLASS_VARIABLE, 
     1013, 1, 47, "550e8400-e29b-41d4-a716-446655440015", "false"},
    
    /* Process Data folder */
    {1020, 1, "ProcessData", "ProcessData", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440020", NULL},
    
    /* Process variables */
    {1021, 1, "Temperature", "Temperature", UA_NODECLASS_VARIABLE, 
     1020, 1, 47, "550e8400-e29b-41d4-a716-446655440021", "23.5"},
    
    {1022, 1, "Pressure", "Pressure", UA_NODECLASS_VARIABLE, 
     1020, 1, 47, "550e8400-e29b-41d4-a716-446655440022", "1.2"},
    
    {1023, 1, "FlowRate", "FlowRate", UA_NODECLASS_VARIABLE, 
     1020, 1, 47, "550e8400-e29b-41d4-a716-446655440023", "15.7"},
    
    /* Diagnostics folder */
    {1030, 1, "Diagnostics", "Diagnostics", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440030", NULL},
    
    /* Diagnostic variables */
    {1031, 1, "SystemHealth", "SystemHealth", UA_NODECLASS_VARIABLE, 
     1030, 1, 47, "550e8400-e29b-41d4-a716-446655440031", "OK"},
    
    {1032, 1, "ErrorCount", "ErrorCount", UA_NODECLASS_VARIABLE, 
     1030, 1, 47, "550e8400-e29b-41d4-a716-446655440032", "0"},
    
    /* Control methods */
    {1040, 1, "ResetSystem", "ResetSystem", UA_NODECLASS_METHOD, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440040", NULL}
};

#define MOCK_BACKEND_NODE_COUNT (sizeof(mockBackendNodes) / sizeof(MockBackendNode))

/* Helper function to find a mock backend node by NodeId */
static const MockBackendNode* findMockBackendNode(UA_UInt32 nodeId, UA_UInt16 namespaceIndex) {
    for(size_t i = 0; i < MOCK_BACKEND_NODE_COUNT; i++) {
        if(mockBackendNodes[i].nodeId == nodeId && 
           mockBackendNodes[i].namespaceIndex == namespaceIndex) {
            return &mockBackendNodes[i];
        }
    }
    return NULL;
}

static void
setAsyncReadResult(UA_Server *server, void *data) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_USERLAND, "read result");
    UA_DataValue *value= (UA_DataValue*)data;
    UA_Server_setAsyncReadResult(server, value);
}

static UA_StatusCode
delayedValueRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                 UA_DataValue *value) {
    /* Get the mockup node */
    const MockBackendNode *mockNode =
        findMockBackendNode(nodeId->identifier.numeric, nodeId->namespaceIndex);
    if(!mockNode)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set the value immediately in the output */
    UA_String v = UA_STRING(mockNode->value);
    UA_Variant_setScalarCopy(&value->value, &v, &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = true;

    /* Return the output with a five second delay */
    UA_DateTime callTime = UA_DateTime_nowMonotonic() + (2 * UA_DATETIME_SEC);
    UA_Server_addTimedCallback(server, setAsyncReadResult, value, callTime, NULL);
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static UA_StatusCode
delayedValueWrite(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *nodeId, void *nodeContext,
                  const UA_NumericRange *range, const UA_DataValue *value) {
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

/**********************/
/* Fallback Nodestore */
/**********************/

typedef struct MapEntry {
    struct MapEntry *orig; /* the version this is a copy from (or NULL) */
    UA_UInt16 refCount; /* How many consumers have a reference to the node? */
    UA_Boolean deleted; /* Node was marked as deleted and can be deleted when refCount == 0 */
    UA_Node node;
} MapEntry;

#define UA_NODEMAP_MINSIZE 64
#define UA_NODEMAP_TOMBSTONE ((MapEntry*)0x01)

typedef struct {
    MapEntry *entry;
    UA_UInt32 nodeIdHash;
} MapSlot;

typedef struct {
    UA_Nodestore ns;

    MapSlot *slots;
    UA_UInt32 size;
    UA_UInt32 count;
    UA_UInt32 sizePrimeIndex;

    /* Maps ReferenceTypeIndex to the NodeId of the ReferenceType */
    UA_NodeId referenceTypeIds[UA_REFERENCETYPESET_MAX];
    UA_Byte referenceTypeCounter;
} AdHocNodestore;

/*********************/
/* HashMap Utilities */
/*********************/

/* The size of the hash-map is always a prime number. They are chosen to be
 * close to the next power of 2. So the size ca. doubles with each prime. */
static UA_UInt32 const primes[] = {
    7,         13,         31,         61,         127,         251,
    509,       1021,       2039,       4093,       8191,        16381,
    32749,     65521,      131071,     262139,     524287,      1048573,
    2097143,   4194301,    8388593,    16777213,   33554393,    67108859,
    134217689, 268435399,  536870909,  1073741789, 2147483647,  4294967291
};

static UA_UInt32 mod(UA_UInt32 h, UA_UInt32 size) { return h % size; }
static UA_UInt32 mod2(UA_UInt32 h, UA_UInt32 size) { return 1 + (h % (size - 2)); }

static UA_UInt16
higher_prime_index(UA_UInt32 n) {
    UA_UInt16 low  = 0;
    UA_UInt16 high = (UA_UInt16)(sizeof(primes) / sizeof(UA_UInt32));
    while(low != high) {
        UA_UInt16 mid = (UA_UInt16)(low + ((high - low) / 2));
        if(n > primes[mid])
            low = (UA_UInt16)(mid + 1);
        else
            high = mid;
    }
    return low;
}

/* Returns an empty slot or null if the nodeid exists or if no empty slot is found. */
static MapSlot *
findFreeSlot(const AdHocNodestore *ns, const UA_NodeId *nodeid) {
    UA_UInt32 h = UA_NodeId_hash(nodeid);
    UA_UInt32 size = ns->size;
    UA_UInt64 idx = mod(h, size); /* Use 64bit container to avoid overflow  */
    UA_UInt32 startIdx = (UA_UInt32)idx;
    UA_UInt32 hash2 = mod2(h, size);

    MapSlot *candidate = NULL;
    do {
        MapSlot *slot = &ns->slots[(UA_UInt32)idx];

        if(slot->entry > UA_NODEMAP_TOMBSTONE) {
            /* A Node with the NodeId does already exist */
            if(slot->nodeIdHash == h &&
               UA_NodeId_equal(&slot->entry->node.head.nodeId, nodeid))
                return NULL;
        } else {
            /* Found a candidate node */
            if(!candidate)
                candidate = slot;
            /* No matching node can come afterwards */
            if(slot->entry == NULL)
                return candidate;
        }

        idx += hash2;
        if(idx >= size)
            idx -= size;
    } while((UA_UInt32)idx != startIdx);

    return candidate;
}

/* The occupancy of the table after the call will be about 50% */
static UA_StatusCode
expand(AdHocNodestore *ns) {
    UA_UInt32 osize = ns->size;
    UA_UInt32 count = ns->count;
    /* Resize only when table after removal of unused elements is either too
       full or too empty */
    if(count * 2 < osize && (count * 8 > osize || osize <= UA_NODEMAP_MINSIZE))
        return UA_STATUSCODE_GOOD;

    MapSlot *oslots = ns->slots;
    UA_UInt32 nindex = higher_prime_index(count * 2);
    UA_UInt32 nsize = primes[nindex];
    MapSlot *nslots= (MapSlot*)UA_calloc(nsize, sizeof(MapSlot));
    if(!nslots)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ns->slots = nslots;
    ns->size = nsize;
    ns->sizePrimeIndex = nindex;

    /* recompute the position of every entry and insert the pointer */
    for(size_t i = 0, j = 0; i < osize && j < count; ++i) {
        if(oslots[i].entry <= UA_NODEMAP_TOMBSTONE)
            continue;
        MapSlot *s = findFreeSlot(ns, &oslots[i].entry->node.head.nodeId);
        UA_assert(s);
        *s = oslots[i];
        ++j;
    }

    UA_free(oslots);
    return UA_STATUSCODE_GOOD;
}

static MapEntry *
createEntry(UA_NodeClass nodeClass) {
    size_t size = sizeof(MapEntry) - sizeof(UA_Node);
    switch(nodeClass) {
    case UA_NODECLASS_OBJECT:
        size += sizeof(UA_ObjectNode);
        break;
    case UA_NODECLASS_VARIABLE:
        size += sizeof(UA_VariableNode);
        break;
    case UA_NODECLASS_METHOD:
        size += sizeof(UA_MethodNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        size += sizeof(UA_ObjectTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        size += sizeof(UA_VariableTypeNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        size += sizeof(UA_ReferenceTypeNode);
        break;
    case UA_NODECLASS_DATATYPE:
        size += sizeof(UA_DataTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        size += sizeof(UA_ViewNode);
        break;
    default:
        return NULL;
    }
    MapEntry *entry = (MapEntry*)UA_calloc(1, size);
    if(!entry)
        return NULL;
    entry->node.head.nodeClass = nodeClass;
    return entry;
}

static void
deleteNodeMapEntry(MapEntry *entry) {
    UA_Node_clear(&entry->node);
    UA_free(entry);
}

static void
cleanupNodeMapEntry(MapEntry *entry) {
    if(entry->refCount > 0)
        return;
    if(entry->deleted) {
        deleteNodeMapEntry(entry);
        return;
    }
    
    /* Check if this is a dynamically created node (orig == NULL indicates backend node) */
    if(entry->orig == NULL && entry->refCount == 0) {
        /* This is a dynamically created node with no more references - mark for deletion */
        entry->deleted = true;
        deleteNodeMapEntry(entry);
        return;
    }
    
    for(size_t i = 0; i < entry->node.head.referencesSize; i++) {
        UA_NodeReferenceKind *rk = &entry->node.head.references[i];
        if(rk->targetsSize > 16 && !rk->hasRefTree)
            UA_NodeReferenceKind_switch(rk);
    }
}

static MapSlot *
findOccupiedSlot(const AdHocNodestore *ns, const UA_NodeId *nodeid) {
    UA_UInt32 h = UA_NodeId_hash(nodeid);
    UA_UInt32 size = ns->size;
    UA_UInt64 idx = mod(h, size); /* Use 64bit container to avoid overflow */
    UA_UInt32 hash2 = mod2(h, size);
    UA_UInt32 startIdx = (UA_UInt32)idx;

    do {
        MapSlot *slot= &ns->slots[(UA_UInt32)idx];
        if(slot->entry > UA_NODEMAP_TOMBSTONE) {
            if(slot->nodeIdHash == h &&
               UA_NodeId_equal(&slot->entry->node.head.nodeId, nodeid))
                return slot;
        } else {
            if(slot->entry == NULL)
                return NULL; /* No further entry possible */
        }

        idx += hash2;
        if(idx >= size)
            idx -= size;
    } while((UA_UInt32)idx != startIdx);

    return NULL;
}

/* Create a UA_Node from mock backend data - simplified version */
static MapEntry* createNodeFromMockData(const MockBackendNode* mockNode) {
    UA_NodeClass nodeClass = mockNode->nodeClass;
    MapEntry *entry = createEntry(nodeClass);
    if(!entry)
        return NULL;
    
    UA_Node *node = &entry->node;
    
    /* Set NodeId */
    node->head.nodeId.namespaceIndex = mockNode->namespaceIndex;
    node->head.nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
    node->head.nodeId.identifier.numeric = mockNode->nodeId;
    
    /* Set BrowseName */
    node->head.browseName.namespaceIndex = mockNode->namespaceIndex;
    node->head.browseName.name = UA_STRING_ALLOC(mockNode->browseName);
    
    /* Set WriteMask */
    node->head.writeMask = 0;
    
    /* Set node-specific attributes */
    switch(nodeClass) {
        case UA_NODECLASS_OBJECT: {
            UA_ObjectNode *objNode = &node->objectNode;
            objNode->eventNotifier = 0;
            break;
        }
        case UA_NODECLASS_VARIABLE: {
            UA_VariableNode *varNode = &node->variableNode;
            varNode->accessLevel = UA_ACCESSLEVELMASK_READ;
            varNode->minimumSamplingInterval = 0.0;
            varNode->historizing = false;
            
            /* Set DataType to String */
            varNode->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_STRING);
            varNode->valueRank = UA_VALUERANK_SCALAR;

            /* Set the value to the async callback */
            varNode->valueSourceType = UA_VALUESOURCETYPE_CALLBACK;
            varNode->valueSource.callback.read = delayedValueRead;
            varNode->valueSource.callback.write = delayedValueWrite;
            break;
        }
        case UA_NODECLASS_METHOD: {
            UA_MethodNode *methodNode = &node->methodNode;
            methodNode->executable = true;
            break;
        }
        default:
            break;
    }
    
    /* Mark this as a dynamically created node */
    entry->orig = NULL; /* No original - this is from backend */
    
    return entry;
}

/***********************/
/* Interface functions */
/***********************/

static UA_Node *
AdHocNodestore_newNode(UA_Nodestore *_, UA_NodeClass nodeClass) {
    MapEntry *entry = createEntry(nodeClass);
    if(!entry)
        return NULL;
    return &entry->node;
}

static void
AdHocNodestore_deleteNode(UA_Nodestore *_, UA_Node *node) {
    MapEntry *entry = container_of(node, MapEntry, node);
    UA_assert(&entry->node == node);
    deleteNodeMapEntry(entry);
}

static const UA_Node *
AdHocNodestore_getNode(UA_Nodestore *orig_ns, const UA_NodeId *nodeid,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    MapSlot *slot = findOccupiedSlot(ns, nodeid);
    
    /* If node exists in hashmap, return it */
    if(slot) {
        ++slot->entry->refCount;
        return &slot->entry->node;
    }
    
    /* Node not in hashmap - check if it exists in mock backend */
    if(nodeid->identifierType == UA_NODEIDTYPE_NUMERIC) {
        const MockBackendNode* mockNode = findMockBackendNode(nodeid->identifier.numeric, 
                                                              nodeid->namespaceIndex);
        if(mockNode) {
            /* Create node from mock data and insert into hashmap */
            MapEntry *entry = createNodeFromMockData(mockNode);
            if(entry) {
                /* Find a free slot and insert */
                MapSlot *newSlot = findFreeSlot(ns, &entry->node.head.nodeId);
                if(newSlot) {
                    newSlot->nodeIdHash = UA_NodeId_hash(&entry->node.head.nodeId);
                    newSlot->entry = entry;
                    ++ns->count;
                    
                    /* Mark as dynamically created for later cleanup */
                    entry->refCount = 1; /* Initial reference */
                    return &entry->node;
                } else {
                    /* Could not insert - cleanup */
                    deleteNodeMapEntry(entry);
                }
            }
        }
    }
    
    return NULL;
}

static const UA_Node *
AdHocNodestore_getNodeFromPtr(UA_Nodestore *ns, UA_NodePointer ptr,
                              UA_UInt32 attributeMask,
                              UA_ReferenceTypeSet references,
                              UA_BrowseDirection referenceDirections) {
    if(!UA_NodePointer_isLocal(ptr))
        return NULL;
    UA_NodeId id = UA_NodePointer_toNodeId(ptr);
    return AdHocNodestore_getNode(ns, &id, attributeMask, references, referenceDirections);
}

static void
AdHocNodestore_releaseNode(UA_Nodestore *orig_ns, const UA_Node *node) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!node)
        return;

    MapEntry *entry = container_of(node, MapEntry, node);
    UA_assert(&entry->node == node);
    UA_assert(entry->refCount > 0);
    --entry->refCount;
    
    /* Special handling for dynamically created nodes */
    if(entry->orig == NULL && entry->refCount == 0) {
        /* This is a dynamically created node with no more references - remove from hashmap */
        MapSlot *slot = findOccupiedSlot(ns, &node->head.nodeId);
        if(slot) {
            slot->entry = UA_NODEMAP_TOMBSTONE;
            --ns->count;
            
            /* Downsize the hashmap if it is very empty */
            if(ns->count * 8 < ns->size && ns->size > UA_NODEMAP_MINSIZE)
                expand(ns); /* Can fail. Just continue with the bigger hashmap. */
        }
        
        /* Delete the entry immediately */
        deleteNodeMapEntry(entry);
    } else {
        cleanupNodeMapEntry(entry);
    }
}

static UA_StatusCode
AdHocNodestore_getNodeCopy(UA_Nodestore *orig_ns, const UA_NodeId *nodeId,
                           UA_Node **outNode) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    MapSlot *slot = findOccupiedSlot(ns, nodeId);
    if(!slot)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    MapEntry *entry = slot->entry;
    MapEntry *newItem = createEntry(entry->node.head.nodeClass);
    if(!newItem)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode retval = UA_Node_copy(&entry->node, &newItem->node);
    if(retval == UA_STATUSCODE_GOOD) {
        newItem->orig = entry; /* Store the pointer to the original */
        *outNode = &newItem->node;
    } else {
        deleteNodeMapEntry(newItem);
    }
    return retval;
}

static UA_StatusCode
AdHocNodestore_removeNode(UA_Nodestore *orig_ns, const UA_NodeId *nodeid) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    MapSlot *slot = findOccupiedSlot(ns, nodeid);
    if(!slot)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    MapEntry *entry = slot->entry;
    slot->entry = UA_NODEMAP_TOMBSTONE;
    entry->deleted = true;
    cleanupNodeMapEntry(entry);
    --ns->count;
    /* Downsize the hashmap if it is very empty */
    if(ns->count * 8 < ns->size && ns->size > UA_NODEMAP_MINSIZE)
        expand(ns); /* Can fail. Just continue with the bigger hashmap. */
    return UA_STATUSCODE_GOOD;
}

/*
 * If this function fails in any way, the node parameter is deleted here,
 * so the caller function does not need to take care of it anymore
 */
static UA_StatusCode
AdHocNodestore_insertNode(UA_Nodestore *orig_ns, UA_Node *node, UA_NodeId *addedNodeId) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(ns->size * 3 <= ns->count * 4) {
        if(expand(ns) != UA_STATUSCODE_GOOD){
            deleteNodeMapEntry(container_of(node, MapEntry, node));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    MapSlot *slot;
    if(node->head.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
       node->head.nodeId.identifier.numeric == 0) {
        /* Create a random nodeid: Start at least with 50,000 to make sure we
         * don not conflict with nodes from the spec. If we find a conflict, we
         * just try another identifier until we have tried all possible
         * identifiers. Since the size is prime and we don't change the increase
         * val, we will reach the starting id again. E.g. adding a nodeset will
         * create children while there are still other nodes which need to be
         * created. Thus the node ids may collide. */
        UA_UInt32 size = ns->size;
        UA_UInt64 identifier = mod(50000 + size+1, UA_UINT32_MAX); /* Use 64bit to
                                                                    * avoid overflow */
        UA_UInt32 increase = mod2(ns->count+1, size);
        UA_UInt32 startId = (UA_UInt32)identifier; /* mod ensures us that the id
                                                    * is a valid 32 bit integer */

        do {
            node->head.nodeId.identifier.numeric = (UA_UInt32)identifier;
            slot = findFreeSlot(ns, &node->head.nodeId);
            if(slot)
                break;
            identifier += increase;
            if(identifier >= size)
                identifier -= size;
#if SIZE_MAX <= UA_UINT32_MAX
            /* The compressed "immediate" representation of nodes does not
             * support the full range on 32bit systems. Generate smaller
             * identifiers as they can be stored more compactly. */
            if(identifier >= (0x01 << 24))
                identifier = identifier % (0x01 << 24);
#endif
        } while((UA_UInt32)identifier != startId);
    } else {
        slot = findFreeSlot(ns, &node->head.nodeId);
    }

    if(!slot) {
        deleteNodeMapEntry(container_of(node, MapEntry, node));
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }

    /* Copy the NodeId */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(addedNodeId) {
        retval = UA_NodeId_copy(&node->head.nodeId, addedNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteNodeMapEntry(container_of(node, MapEntry, node));
            return retval;
        }
    }

    /* For new ReferencetypeNodes add to the index map */
    if(node->head.nodeClass == UA_NODECLASS_REFERENCETYPE) {
        UA_ReferenceTypeNode *refNode = &node->referenceTypeNode;
        if(ns->referenceTypeCounter >= UA_REFERENCETYPESET_MAX) {
            deleteNodeMapEntry(container_of(node, MapEntry, node));
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        retval = UA_NodeId_copy(&node->head.nodeId, &ns->referenceTypeIds[ns->referenceTypeCounter]);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteNodeMapEntry(container_of(node, MapEntry, node));
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        /* Assign the ReferenceTypeIndex to the new ReferenceTypeNode */
        refNode->referenceTypeIndex = ns->referenceTypeCounter;
        refNode->subTypes = UA_REFTYPESET(ns->referenceTypeCounter);

        ns->referenceTypeCounter++;
    }

    /* Insert the node */
    MapEntry *newEntry = container_of(node, MapEntry, node);
    slot->nodeIdHash = UA_NodeId_hash(&node->head.nodeId);
    slot->entry = newEntry;
    ++ns->count;
    return retval;
}

static UA_StatusCode
AdHocNodestore_replaceNode(UA_Nodestore *orig_ns, UA_Node *node) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    MapEntry *newEntry = container_of(node, MapEntry, node);

    /* Find the node */
    MapSlot *slot = findOccupiedSlot(ns, &node->head.nodeId);
    if(!slot) {
        deleteNodeMapEntry(newEntry);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* The node was already updated since the copy was made? */
    MapEntry *oldEntry = slot->entry;
    if(oldEntry != newEntry->orig) {
        deleteNodeMapEntry(newEntry);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Replace the entry */
    slot->entry = newEntry;
    oldEntry->deleted = true;
    cleanupNodeMapEntry(oldEntry);
    return UA_STATUSCODE_GOOD;
}

static const UA_NodeId *
AdHocNodestore_getReferenceTypeId(UA_Nodestore *orig_ns, UA_Byte refTypeIndex) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(refTypeIndex >= ns->referenceTypeCounter)
        return NULL;
    return &ns->referenceTypeIds[refTypeIndex];
}

static void
AdHocNodestore_iterate(UA_Nodestore *orig_ns, UA_NodestoreVisitor visitor,
                       void *visitorContext) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    for(UA_UInt32 i = 0; i < ns->size; ++i) {
        MapSlot *slot = &ns->slots[i];
        if(slot->entry > UA_NODEMAP_TOMBSTONE) {
            /* The visitor can delete the node. So refcount here. */
            slot->entry->refCount++;
            visitor(visitorContext, &slot->entry->node);
            slot->entry->refCount--;
            cleanupNodeMapEntry(slot->entry);
        }
    }
}

static void
AdHocNodestore_free(UA_Nodestore *ns) {
    AdHocNodestore *ans = (AdHocNodestore*)ns;

    UA_UInt32 size = ans->size;
    MapSlot *slots = ans->slots;
    for(UA_UInt32 i = 0; i < size; ++i) {
        if(slots[i].entry > UA_NODEMAP_TOMBSTONE) {
            /* On debugging builds, check that all nodes were release */
            UA_assert(slots[i].entry->refCount == 0);
            /* Delete the node */
            deleteNodeMapEntry(slots[i].entry);
        }
    }
    UA_free(ans->slots);

    /* Clean up the ReferenceTypes index array */
    for(size_t i = 0; i < ans->referenceTypeCounter; i++)
        UA_NodeId_clear(&ans->referenceTypeIds[i]);

    UA_free(ans);
}

UA_Nodestore *
UA_Nodestore_PilzAdHoc(void) {
    /* Allocate and initialize the nodemap */
    AdHocNodestore *ns = (AdHocNodestore*)UA_malloc(sizeof(AdHocNodestore));
    if(!ns)
        return NULL;

    ns->referenceTypeCounter = 0;
    ns->sizePrimeIndex = higher_prime_index(UA_NODEMAP_MINSIZE);
    ns->size = primes[ns->sizePrimeIndex];
    ns->count = 0;
    ns->slots = (MapSlot*)
        UA_calloc(ns->size, sizeof(MapSlot));
    if(!ns->slots) {
        UA_free(ns);
        return NULL;
    }

    /* Populate the nodestore */
    ns->ns.free = AdHocNodestore_free;
    ns->ns.newNode = AdHocNodestore_newNode;
    ns->ns.deleteNode = AdHocNodestore_deleteNode;
    ns->ns.getNode = AdHocNodestore_getNode;
    ns->ns.getNodeFromPtr = AdHocNodestore_getNodeFromPtr;
    ns->ns.releaseNode = AdHocNodestore_releaseNode;
    ns->ns.getNodeCopy = AdHocNodestore_getNodeCopy;
    ns->ns.insertNode = AdHocNodestore_insertNode;
    ns->ns.replaceNode = AdHocNodestore_replaceNode;
    ns->ns.removeNode = AdHocNodestore_removeNode;
    ns->ns.getReferenceTypeId = AdHocNodestore_getReferenceTypeId;
    ns->ns.iterate = AdHocNodestore_iterate;

    /* All nodes are stored in RAM. Changes are made in-situ. GetEditNode is
     * identical to GetNode -- but the Node pointer is non-const. */
    ns->ns.getEditNode =
        (UA_Node * (*)(UA_Nodestore *ns, const UA_NodeId *nodeId,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))AdHocNodestore_getNode;
    ns->ns.getEditNodeFromPtr =
        (UA_Node * (*)(UA_Nodestore *ns, UA_NodePointer ptr,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))AdHocNodestore_getNodeFromPtr;

    return &ns->ns;
}
