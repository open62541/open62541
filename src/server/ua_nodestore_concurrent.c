#include "ua_util.h"
#include "ua_nodestore.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_MULTITHREADING /* conditional compilation */

struct nodeEntry {
    struct cds_lfht_node htn; ///< Contains the next-ptr for urcu-hashmap
    struct rcu_head rcu_head; ///< For call-rcu
    struct nodeEntry *orig; //< the version this is a copy from (or NULL)
    UA_Node node; ///< Might be cast from any _bigger_ UA_Node* type. Allocate enough memory!
};

#include "ua_nodestore_hash.inc"

static struct nodeEntry * instantiateEntry(UA_NodeClass class) {
    size_t size = sizeof(struct nodeEntry) - sizeof(UA_Node);
    switch(class) {
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
    struct nodeEntry *entry = UA_calloc(1, size);
    if(!entry)
        return NULL;
    entry->node.nodeClass = class;
    return entry;
}

static void deleteEntry(struct rcu_head *head) {
    struct nodeEntry *entry = container_of(head, struct nodeEntry, rcu_head);
    UA_Node_deleteMembersAnyNodeClass(&entry->node);
    UA_free(entry);
}

/* We are in a rcu_read lock. So the node will not be freed under our feet. */
static int compare(struct cds_lfht_node *htn, const void *orig) {
    const UA_NodeId *origid = (const UA_NodeId *)orig;
    /* The htn is first in the entry structure. */
    const UA_NodeId *newid  = &((struct nodeEntry *)htn)->node.nodeId;
    return UA_NodeId_equal(newid, origid);
}

UA_NodeStore * UA_NodeStore_new() {
    /* 64 is the minimum size for the hashtable. */
    return (UA_NodeStore*)cds_lfht_new(64, 64, 0, CDS_LFHT_AUTO_RESIZE, NULL);
}

/* do not call with read-side critical section held!! */
void UA_NodeStore_delete(UA_NodeStore *ns) {
    UA_ASSERT_RCU_LOCKED();
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    struct cds_lfht_iter iter;
    cds_lfht_first(ht, &iter);
    while(iter.node) {
        if(!cds_lfht_del(ht, iter.node)) {
            /* points to the htn entry, which is first */
            struct nodeEntry *entry = (struct nodeEntry*) iter.node;
            call_rcu(&entry->rcu_head, deleteEntry);
        }
        cds_lfht_next(ht, &iter);
    }
    cds_lfht_destroy(ht, NULL);
    UA_free(ns);
}

UA_Node * UA_NodeStore_newNode(UA_NodeClass class) {
    struct nodeEntry *entry = instantiateEntry(class);
    if(!entry)
        return NULL;
    return (UA_Node*)&entry->node;
}

void UA_NodeStore_deleteNode(UA_Node *node) {
    struct nodeEntry *entry = container_of(node, struct nodeEntry, node);
    deleteEntry(&entry->rcu_head);
}

UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node) {
    UA_ASSERT_RCU_LOCKED();
    struct nodeEntry *entry = container_of(node, struct nodeEntry, node);
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    cds_lfht_node_init(&entry->htn);
    struct cds_lfht_node *result;
    //namespace index is assumed to be valid
    UA_NodeId tempNodeid;
    tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    if(!UA_NodeId_isNull(&tempNodeid)) {
        hash_t h = hash(&node->nodeId);
        result = cds_lfht_add_unique(ht, h, compare, &node->nodeId, &entry->htn);
        /* If the nodeid exists already */
        if(result != &entry->htn) {
            deleteEntry(&entry->rcu_head);
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    } else {
        /* create a unique nodeid */
        node->nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
        if(node->nodeId.namespaceIndex == 0) // original request for ns=0 should yield ns=1
            node->nodeId.namespaceIndex = 1;

        unsigned long identifier;
        long before, after;
        cds_lfht_count_nodes(ht, &before, &identifier, &after); // current number of nodes stored
        identifier++;

        node->nodeId.identifier.numeric = (UA_UInt32)identifier;
        while(true) {
            hash_t h = hash(&node->nodeId);
            result = cds_lfht_add_unique(ht, h, compare, &node->nodeId, &entry->htn);
            if(result == &entry->htn)
                break;
            node->nodeId.identifier.numeric += (UA_UInt32)(identifier * 2654435761);
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, UA_Node *node) {
    UA_ASSERT_RCU_LOCKED();
    struct nodeEntry *entry = container_of(node, struct nodeEntry, node);
    struct cds_lfht *ht = (struct cds_lfht*)ns;

    /* Get the current version */
    hash_t h = hash(&node->nodeId);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ht, h, compare, &node->nodeId, &iter);
    if(!iter.node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* We try to replace an obsolete version of the node */
    struct nodeEntry *oldEntry = (struct nodeEntry*)iter.node;
    if(oldEntry != entry->orig)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    cds_lfht_node_init(&entry->htn);
    if(cds_lfht_replace(ht, &iter, h, compare, &node->nodeId, &entry->htn) != 0) {
        /* Replacing failed. Maybe the node got replaced just before this thread tried to.*/
        deleteEntry(&entry->rcu_head);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
        
    /* If an entry got replaced, mark it as dead. */
    call_rcu(&oldEntry->rcu_head, deleteEntry);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    UA_ASSERT_RCU_LOCKED();
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    hash_t h = hash(nodeid);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ht, h, compare, nodeid, &iter);
    if(!iter.node || cds_lfht_del(ht, iter.node) != 0)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    struct nodeEntry *entry = (struct nodeEntry*)iter.node;
    call_rcu(&entry->rcu_head, deleteEntry);
    return UA_STATUSCODE_GOOD;
}

const UA_Node * UA_NodeStore_get(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    UA_ASSERT_RCU_LOCKED();
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    hash_t h = hash(nodeid);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ht, h, compare, nodeid, &iter);
    struct nodeEntry *found_entry = (struct nodeEntry*)iter.node;
    if(!found_entry)
        return NULL;
    return &found_entry->node;
}

UA_Node * UA_NodeStore_getCopy(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    UA_ASSERT_RCU_LOCKED();
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    hash_t h = hash(nodeid);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ht, h, compare, nodeid, &iter);
    struct nodeEntry *entry = (struct nodeEntry*)iter.node;
    if(!entry)
        return NULL;
    struct nodeEntry *new = instantiateEntry(entry->node.nodeClass);
    if(!new)
        return NULL;
    if(UA_Node_copyAnyNodeClass(&entry->node, &new->node) != UA_STATUSCODE_GOOD) {
        deleteEntry(&new->rcu_head);
        return NULL;
    }
    new->orig = entry;
    return &new->node;
}

void UA_NodeStore_iterate(UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor) {
    UA_ASSERT_RCU_LOCKED();
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    struct cds_lfht_iter iter;
    cds_lfht_first(ht, &iter);
    while(iter.node != NULL) {
        struct nodeEntry *found_entry = (struct nodeEntry*)iter.node;
        visitor(&found_entry->node);
        cds_lfht_next(ht, &iter);
    }
}

#endif /* UA_ENABLE_MULTITHREADING */
