#include "ua_util.h"
#include "ua_nodestore.h"

#define ALIVE_BIT (1 << 15) /* Alive bit in the refcount */

struct nodeEntry {
    struct cds_lfht_node htn;      /* contains next-ptr for urcu-hashmap */
    struct rcu_head      rcu_head; /* For call-rcu */
    UA_UInt16 refcount;            /* Counts the amount of readers on it [alive-bit, 15 counter-bits] */
    UA_Node node;                  /* Might be cast from any _bigger_ UA_Node* type. Allocate enough memory! */
};

struct UA_NodeStore {
    struct cds_lfht *ht; /* Hash table */
};

#include "ua_nodestore_hash.inc"

static void node_deleteMembers(UA_Node *node) {
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        UA_ObjectNode_deleteMembers((UA_ObjectNode *)node);
        break;
    case UA_NODECLASS_VARIABLE:
        UA_VariableNode_deleteMembers((UA_VariableNode *)node);
        break;
    case UA_NODECLASS_METHOD:
        UA_MethodNode_deleteMembers((UA_MethodNode *)node);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        UA_ObjectTypeNode_deleteMembers((UA_ObjectTypeNode *)node);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        UA_VariableTypeNode_deleteMembers((UA_VariableTypeNode *)node);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        UA_ReferenceTypeNode_deleteMembers((UA_ReferenceTypeNode *)node);
        break;
    case UA_NODECLASS_DATATYPE:
        UA_DataTypeNode_deleteMembers((UA_DataTypeNode *)node);
        break;
    case UA_NODECLASS_VIEW:
        UA_ViewNode_deleteMembers((UA_ViewNode *)node);
        break;
    default:
        UA_assert(UA_FALSE);
        break;
    }
}

/* We are in a rcu_read lock. So the node will not be freed under our feet. */
static int compare(struct cds_lfht_node *htn, const void *orig) {
    const UA_NodeId *origid = (const UA_NodeId *)orig;
    const UA_NodeId *newid  = &((struct nodeEntry *)htn)->node.nodeId;   /* The htn is first in the entry structure. */
    return UA_NodeId_equal(newid, origid);
}

/* The entry was removed from the hashtable. No more readers can get it. Since
   all readers using the node for a longer time (outside the rcu critical
   section) increased the refcount, we only need to wait for the refcount
   to reach zero. */
static void markDead(struct rcu_head *head) {
    struct nodeEntry *entry = (struct nodeEntry*) ((uintptr_t)head - offsetof(struct nodeEntry, rcu_head)); 
    uatomic_and(&entry->refcount, ~ALIVE_BIT); // set the alive bit to zero
    if(uatomic_read(&entry->refcount) > 0)
        return;

    node_deleteMembers(&entry->node);
    UA_free(entry);
}

/* Free the entry if it is dead and nobody uses it anymore */
void UA_NodeStore_release(const UA_Node *managed) {
    struct nodeEntry *entry = (struct nodeEntry*) ((uintptr_t)managed - offsetof(struct nodeEntry, node)); 
    if(uatomic_add_return(&entry->refcount, -1) == 0) {
        node_deleteMembers(&entry->node);
        UA_free(entry);
    }
}

UA_NodeStore * UA_NodeStore_new() {
    UA_NodeStore *ns;
    if(!(ns = UA_malloc(sizeof(UA_NodeStore))))
        return UA_NULL;

    /* 32 is the minimum size for the hashtable. */
    ns->ht = cds_lfht_new(32, 32, 0, CDS_LFHT_AUTO_RESIZE, NULL);
    if(!ns->ht) {
        UA_free(ns);
        return UA_NULL;
    }
    return ns;
}

void UA_NodeStore_delete(UA_NodeStore *ns) {
    struct cds_lfht      *ht = ns->ht;
    struct cds_lfht_iter  iter;

    rcu_read_lock();
    cds_lfht_first(ht, &iter);
    while(iter.node) {
        if(!cds_lfht_del(ht, iter.node)) {
            struct nodeEntry *entry = (struct nodeEntry*) ((uintptr_t)iter.node - offsetof(struct nodeEntry, htn)); 
            call_rcu(&entry->rcu_head, markDead);
        }
        cds_lfht_next(ht, &iter);
    }
    rcu_read_unlock();
    cds_lfht_destroy(ht, UA_NULL);

    UA_free(ns);
}

UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node, const UA_Node **inserted) {
    size_t nodesize;
    /* Copy the node into the entry. Then reset the original node. It shall no longer be used. */
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        nodesize = sizeof(UA_ObjectNode);
        break;
    case UA_NODECLASS_VARIABLE:
        nodesize = sizeof(UA_VariableNode);
        break;
    case UA_NODECLASS_METHOD:
        nodesize = sizeof(UA_MethodNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        nodesize = sizeof(UA_ObjectTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        nodesize = sizeof(UA_VariableTypeNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        nodesize = sizeof(UA_ReferenceTypeNode);
        break;
    case UA_NODECLASS_DATATYPE:
        nodesize = sizeof(UA_DataTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        nodesize = sizeof(UA_ViewNode);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    struct nodeEntry *entry;
    if(!(entry = UA_malloc(sizeof(struct nodeEntry) - sizeof(UA_Node) + nodesize)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_memcpy((void*)&entry->node, node, nodesize);

    cds_lfht_node_init(&entry->htn);
    entry->refcount = ALIVE_BIT;
    if(inserted) // increase the counter before adding the node
        entry->refcount++;

    struct cds_lfht_node *result;
    //FIXME: a bit dirty workaround of preserving namespace
    //namespace index is assumed to be valid
    UA_NodeId tempNodeid;
    UA_NodeId_copy(&node->nodeId, &tempNodeid);
    tempNodeid.namespaceIndex = 0;
    if(!UA_NodeId_isNull(&tempNodeid)) {
        hash_t h = hash(&node->nodeId);
        rcu_read_lock();
        result = cds_lfht_add_unique(ns->ht, h, compare, &entry->node.nodeId, &entry->htn);
        rcu_read_unlock();

        /* If the nodeid exists already */
        if(result != &entry->htn) {
            UA_free(entry);
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    } else {
        /* create a unique nodeid */
        ((UA_Node *)&entry->node)->nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
        if(((UA_Node *)&entry->node)->nodeId.namespaceIndex == 0) //original request for ns=0 should yield ns=1
            ((UA_Node *)&entry->node)->nodeId.namespaceIndex = 1;
        if(((UA_Node *)&entry->node)->nodeClass==UA_NODECLASS_VARIABLE){ //set namespaceIndex in browseName in case id is generated
        	((UA_VariableNode*)&entry->node)->browseName.namespaceIndex=((UA_Node *)&entry->node)->nodeId.namespaceIndex;
        }
        //set namespaceIndex in browseName in case id is generated
        ((UA_Node *)&entry->node)->browseName.namespaceIndex=node->nodeId.namespaceIndex;

        unsigned long identifier;
        long before, after;
        rcu_read_lock();
        cds_lfht_count_nodes(ns->ht, &before, &identifier, &after); // current amount of nodes stored
        identifier++;

        ((UA_Node *)&entry->node)->nodeId.identifier.numeric = identifier;
        while(UA_TRUE) {
            hash_t nhash = hash(&entry->node.nodeId);
            result = cds_lfht_add_unique(ns->ht, nhash, compare, &entry->node.nodeId, &entry->htn);
            if(result == &entry->htn)
                break;

            ((UA_Node *)&entry->node)->nodeId.identifier.numeric += (identifier * 2654435761);
        }
        rcu_read_unlock();
    }

    UA_free(node);
    if(inserted)
        *inserted = &entry->node;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, const UA_Node *oldNode, UA_Node *node,
                                   const UA_Node **inserted) {
    size_t nodesize;
    /* Copy the node into the entry. Then reset the original node. It shall no longer be used. */
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        nodesize = sizeof(UA_ObjectNode);
        break;
    case UA_NODECLASS_VARIABLE:
        nodesize = sizeof(UA_VariableNode);
        break;
    case UA_NODECLASS_METHOD:
        nodesize = sizeof(UA_MethodNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        nodesize = sizeof(UA_ObjectTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        nodesize = sizeof(UA_VariableTypeNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        nodesize = sizeof(UA_ReferenceTypeNode);
        break;
    case UA_NODECLASS_DATATYPE:
        nodesize = sizeof(UA_DataTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        nodesize = sizeof(UA_ViewNode);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    struct nodeEntry *newEntry;
    if(!(newEntry = UA_malloc(sizeof(struct nodeEntry) - sizeof(UA_Node) + nodesize)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_memcpy((void*)&newEntry->node, node, nodesize);

    cds_lfht_node_init(&newEntry->htn);
    newEntry->refcount = ALIVE_BIT;
    if(inserted) // increase the counter before adding the node
        newEntry->refcount++;

    hash_t h = hash(&node->nodeId);
    struct cds_lfht_iter iter;
    rcu_read_lock();
    cds_lfht_lookup(ns->ht, h, compare, &node->nodeId, &iter);

    /* No node found that can be replaced */
    if(!iter.node) {
        rcu_read_unlock();
        UA_free(newEntry);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    struct nodeEntry *oldEntry = (struct nodeEntry*) ((uintptr_t)iter.node - offsetof(struct nodeEntry, htn)); 
    /* The node we found is obsolete*/
    if(&oldEntry->node != oldNode) {
        rcu_read_unlock();
        UA_free(newEntry);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* The old node is replaced by a managed node. */
    if(cds_lfht_replace(ns->ht, &iter, h, compare, &node->nodeId, &newEntry->htn) != 0) {
        /* Replacing failed. Maybe the node got replaced just before this thread tried to.*/
        rcu_read_unlock();
        UA_free(newEntry);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
        
    /* If an entry got replaced, mark it as dead. */
    call_rcu(&oldEntry->rcu_head, markDead);
    rcu_read_unlock();
    UA_free(node);

    if(inserted)
        *inserted = &newEntry->node;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    hash_t nhash = hash(nodeid);
    struct cds_lfht_iter iter;

    rcu_read_lock();
    /* If this fails, then the node has already been removed. */
    cds_lfht_lookup(ns->ht, nhash, compare, &nodeid, &iter);
    if(!iter.node || cds_lfht_del(ns->ht, iter.node) != 0) {
        rcu_read_unlock();
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    struct nodeEntry *entry = (struct nodeEntry*) ((uintptr_t)iter.node - offsetof(struct nodeEntry, htn)); 
    call_rcu(&entry->rcu_head, markDead);
    rcu_read_unlock();

    return UA_STATUSCODE_GOOD;
}

const UA_Node * UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid) {
    hash_t nhash = hash(nodeid);
    struct cds_lfht_iter iter;

    rcu_read_lock();
    cds_lfht_lookup(ns->ht, nhash, compare, nodeid, &iter);
    struct nodeEntry *found_entry = (struct nodeEntry *)cds_lfht_iter_get_node(&iter);

    if(!found_entry) {
        rcu_read_unlock();
        return UA_NULL;
    }

    /* This is done within a read-lock. The node will not be marked dead within a read-lock. */
    uatomic_inc(&found_entry->refcount);
    rcu_read_unlock();
    return &found_entry->node;
}

void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor) {
    struct cds_lfht     *ht = ns->ht;
    struct cds_lfht_iter iter;

    rcu_read_lock();
    cds_lfht_first(ht, &iter);
    while(iter.node != UA_NULL) {
        struct nodeEntry *found_entry = (struct nodeEntry *)cds_lfht_iter_get_node(&iter);
        uatomic_inc(&found_entry->refcount);
        const UA_Node      *node = &found_entry->node;
        rcu_read_unlock();
        visitor(node);
        UA_NodeStore_release((const UA_Node *)node);
        rcu_read_lock();
        cds_lfht_next(ht, &iter);
    }
    rcu_read_unlock();
}
