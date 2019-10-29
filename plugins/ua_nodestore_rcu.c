/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2014-2017, 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/plugin/nodestore_default.h>

#ifdef __linux__

#include <urcu/rculfhash.h>

struct nodeEntry {
    struct cds_lfht_node htn;
    struct rcu_head rcu_head; /* For call-rcu */
    UA_UInt16 refCount;       /* How many consumers have a reference to the node? */
    UA_Boolean deleted;       /* Node was marked as deleted and can be deleted when refCount == 0 */
    struct nodeEntry *orig;   /* the version this is a copy from (or NULL) */
    UA_NodeId nodeId;         /* This is actually a UA_Node that also starts with a NodeId */
};

static struct nodeEntry *
instantiateEntry(UA_NodeClass nc) {
    size_t size = sizeof(struct nodeEntry) - sizeof(UA_NodeId);
    switch(nc) {
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
    struct nodeEntry *entry = (struct nodeEntry*)UA_calloc(1, size);
    if(!entry)
        return NULL;
    entry->node.nodeClass = nc;
    return entry;
}

static void
deleteEntryRCU(struct rcu_head *head) {
    struct nodeEntry *entry = container_of(head, struct nodeEntry, rcu_head);
    UA_Node_clear((UA_Node*)&entry->nodeId);
    UA_free(entry);
}

static void
cleanupEntryRCU(struct rcu_head *head) {
    struct nodeEntry *entry = container_of(head, struct nodeEntry, rcu_head);
    if(entry->deleted && entry->refCount == 0)
        deleteEntryRCU(entry);
}

/* We are in a rcu_read lock. So the node will not be freed under our feet. */
static int
compareRCU(struct cds_lfht_node *htn, const void *orig) {
    const UA_NodeId *origid = (const UA_NodeId *)orig;
    /* The htn is first in the entry structure. */
    const UA_NodeId *newid  = &((struct nodeEntry *)htn)->nodeId;
    return UA_NodeId_equal(newid, origid);
}

static UA_Node *
nsRCU_newNode(void *nsCtx, UA_NodeClass nc) {
    struct nodeEntry *entry = instantiateEntry(nc);
    if(!entry)
        return NULL;
    return (UA_Node*)&entry->node;
}

static void
nsRCU_deleteNode(UA_Node *node) {
    struct nodeEntry *entry = container_of(&node->nodeId, struct nodeEntry, nodeId);
    deleteEntryRCU(&entry->rcu_head);
}

static const UA_Node *
nsRCU_getNode(void *nsCtx, const UA_NodeId *nodeid) {
    struct cds_lfht *ht = (struct cds_lfht*)nsCtx;
    UA_UInt32 h = UA_NodeId_hash(nodeid);
    struct cds_lfht_iter iter;
    rcu_read_lock();
    cds_lfht_lookup(ht, h, compare, nodeid, &iter);
    struct nodeEntry *found_entry = (struct nodeEntry*)iter.node;
    if(!found_entry) {
        rcu_read_unlock();
        return NULL;
    }
    found_entry->refCount++; /* increase the counter */
    rcu_read_unlock();
    return &found_entry->node;
}

static void
nsRCU_releaseNode(void *nsCtx, const UA_Node *node) {
    if(!node)
        return;
    struct nodeEntry *entry = container_of(&node->nodeId, struct nodeEntry, nodeId);
    --entry->refCount;
    cleanupEntry(entry);
}

static UA_StatusCode
nsRCU_insertNode(void *nsCtx, UA_Node *node) {
    struct nodeEntry *entry = container_of(node, struct nodeEntry, node);
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    cds_lfht_node_init(&entry->htn);
    struct cds_lfht_node *result;
    //namespace index is assumed to be valid
    UA_NodeId tempNodeid;
    tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    if(!UA_NodeId_isNull(&tempNodeid)) {
        UA_UInt32 h = UA_NodeId_hash(&node->nodeId);
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
        ++identifier;

        node->nodeId.identifier.numeric = (UA_UInt32)identifier;
        while(true) {
            UA_UInt32 h = UA_NodeId_hash(&node->nodeId);
            result = cds_lfht_add_unique(ht, h, compare, &node->nodeId, &entry->htn);
            if(result == &entry->htn)
                break;
            node->nodeId.identifier.numeric += (UA_UInt32)(identifier * 2654435761);
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_Node *
nsRCU_getNodeCopy(void *nsCtx, const UA_NodeId *nodeId) {
    struct cds_lfht *ht = (struct cds_lfht*)nsCtx;
    UA_UInt32 h = UA_NodeId_hash(nodeid);
    struct cds_lfht_iter iter;
    rcu_read_lock();
    cds_lfht_lookup(ht, h, compare, nodeid, &iter);
    struct nodeEntry *entry = (struct nodeEntry*)iter.node;
    if(!entry) {
        rcu_read_unlock();
        return NULL;
    }
    struct nodeEntry *n = instantiateEntry(entry->node.nodeClass);
    if(!n) {
        rcu_read_unlock();
        return NULL;
    }
    if(UA_Node_copyAnyNodeClass(&entry->node, &n->node) != UA_STATUSCODE_GOOD) {
        rcu_read_unlock();
        deleteEntry(&n->rcu_head);
        return NULL;
    }
    rcu_read_unlock();
    n->orig = entry;
    return &n->node;
}

UA_StatusCode UA_Nodestore_replace(UA_Nodestore *ns, UA_Node *node) {
    struct nodeEntry *entry = container_of(node, struct nodeEntry, node);
    struct cds_lfht *ht = (struct cds_lfht*)ns;

    /* Get the current version */
    UA_UInt32 h = UA_NodeId_hash(&node->nodeId);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ht, h, compare, &node->nodeId, &iter);
    if(!iter.node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* We try to replace an obsolete version of the node */
    struct nodeEntry *oldEntry = (struct nodeEntry*)iter.node;
    if(oldEntry != entry->orig) {
        deleteEntry(&entry->rcu_head);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
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

UA_StatusCode UA_Nodestore_remove(UA_Nodestore *ns, const UA_NodeId *nodeid) {
    struct cds_lfht *ht = (struct cds_lfht*)ns;
    UA_UInt32 h = UA_NodeId_hash(nodeid);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ht, h, compare, nodeid, &iter);
    if(!iter.node || cds_lfht_del(ht, iter.node) != 0)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    struct nodeEntry *entry = (struct nodeEntry*)iter.node;
    call_rcu(&entry->rcu_head, deleteEntry);
    return UA_STATUSCODE_GOOD;
}

static void
nsRCU_iterate(void *nsCtx, UA_Nodestore_nodeVisitor visitor) {
    struct cds_lfht *ht = (struct cds_lfht*)nsCtx;
    rcu_read_lock();
    struct cds_lfht_iter iter;
    cds_lfht_first(ht, &iter);
    while(iter.node != NULL) {
        struct nodeEntry *found_entry = (struct nodeEntry*)iter.node;
        found_entry->refCount++;
        visitor(&found_entry->node);
        cds_lfht_next(ht, &iter);
    }
    rcu_read_unlock();
}

/* do not call with read-side critical section held!! */
void
nsRCU_clear(void *nsCtx) {
    struct cds_lfht *ht = (struct cds_lfht*)nsCtx;
    struct cds_lfht_iter iter;
    rcu_read_lock();
    cds_lfht_first(ht, &iter);
    while(iter.node) {
        if(!cds_lfht_del(ht, iter.node)) {
            /* points to the htn entry, which is first */
            struct nodeEntry *entry = (struct nodeEntry*) iter.node;
            call_rcu(&entry->rcu_head, deleteEntry);
        }
        cds_lfht_next(ht, &iter);
    }
    rcu_read_unlock();
    cds_lfht_destroy(ht, NULL);
}

UA_StatusCode
UA_Nodestore_RCU(UA_Nodestore *ns) {
    /* Allocate and initialize the context */
    ns->context = cds_lfht_new(64, 64, 0, CDS_LFHT_AUTO_RESIZE, NULL);
    if(!ns->context)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Populate the nodestore */
    ns->clear = nsRCU_clear
    ns->newNode = nsRCU_newNode;
    ns->deleteNode = nsRCU_deleteNode;
    ns->getNode = nsRCU_getNode;
    ns->releaseNode = nsRCU_releaseNode;
    ns->getNodeCopy = nsRCU_getNodeCopy;
    ns->insertNode = nsRCU_insertNode;
    ns->replaceNode = zipNsReplaceNode;
    ns->removeNode = zipNsRemoveNode;
    ns->iterate = nsRCU_iterate;
    return UA_STATUSCODE_GOOD;
}

#endif /* __linux__  */
