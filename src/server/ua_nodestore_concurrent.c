#include "ua_util.h"
#include "ua_nodestore.h"

struct nodeEntry {
    struct cds_lfht_node htn; ///< Contains the next-ptr for urcu-hashmap
    struct rcu_head rcu_head; ///< For call-rcu
    UA_Node node; ///< Might be cast from any _bigger_ UA_Node* type. Allocate enough memory!
};

struct UA_NodeStore {
    struct cds_lfht *ht;
};

#include "ua_nodestore_hash.inc"

static void deleteEntry(struct rcu_head *head) {
    struct nodeEntry *entry = caa_container_of(head, struct nodeEntry, rcu_head);
    switch(entry->node.nodeClass) {
    case UA_NODECLASS_OBJECT:
        UA_ObjectNode_deleteMembers((UA_ObjectNode*)&entry->node);
        break;
    case UA_NODECLASS_VARIABLE:
        UA_VariableNode_deleteMembers((UA_VariableNode*)&entry->node);
        break;
    case UA_NODECLASS_METHOD:
        UA_MethodNode_deleteMembers((UA_MethodNode*)&entry->node);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        UA_ObjectTypeNode_deleteMembers((UA_ObjectTypeNode*)&entry->node);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        UA_VariableTypeNode_deleteMembers((UA_VariableTypeNode*)&entry->node);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        UA_ReferenceTypeNode_deleteMembers((UA_ReferenceTypeNode*)&entry->node);
        break;
    case UA_NODECLASS_DATATYPE:
        UA_DataTypeNode_deleteMembers((UA_DataTypeNode*)&entry->node);
        break;
    case UA_NODECLASS_VIEW:
        UA_ViewNode_deleteMembers((UA_ViewNode*)&entry->node);
        break;
    default:
        UA_assert(UA_FALSE);
        break;
    }
    free(entry);
}

/* We are in a rcu_read lock. So the node will not be freed under our feet. */
static int compare(struct cds_lfht_node *htn, const void *orig) {
    const UA_NodeId *origid = (const UA_NodeId *)orig;
    /* The htn is first in the entry structure. */
    const UA_NodeId *newid  = &((struct nodeEntry *)htn)->node.nodeId;
    return UA_NodeId_equal(newid, origid);
}

UA_NodeStore * UA_NodeStore_new() {
    UA_NodeStore *ns;
    if(!(ns = UA_malloc(sizeof(UA_NodeStore))))
        return NULL;

    /* 32 is the minimum size for the hashtable. */
    ns->ht = cds_lfht_new(32, 32, 0, CDS_LFHT_AUTO_RESIZE, NULL);
    if(!ns->ht) {
        UA_free(ns);
        ns = NULL;
    }
    return ns;
}

/* do not call with read-side critical section held!! */
void UA_NodeStore_delete(UA_NodeStore *ns) {
    struct cds_lfht *ht = ns->ht;
    struct cds_lfht_iter  iter;
    cds_lfht_first(ht, &iter);
    rcu_read_lock();
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
    UA_Node *newNode = &entry->node;
    memcpy(newNode, node, nodesize);

    cds_lfht_node_init(&entry->htn);
    struct cds_lfht_node *result;
    //namespace index is assumed to be valid
    UA_NodeId tempNodeid;
    tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    if(!UA_NodeId_isNull(&tempNodeid)) {
        hash_t h = hash(&node->nodeId);
        result = cds_lfht_add_unique(ns->ht, h, compare, &newNode->nodeId, &entry->htn);
        /* If the nodeid exists already */
        if(result != &entry->htn) {
            UA_free(entry);
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    } else {
        /* create a unique nodeid */
        newNode->nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
        if(newNode->nodeId.namespaceIndex == 0) // original request for ns=0 should yield ns=1
            newNode->nodeId.namespaceIndex = 1;
        /* set namespaceIndex in browseName in case id is generated */
        if(newNode->nodeClass == UA_NODECLASS_VARIABLE)
        	((UA_VariableNode*)newNode)->browseName.namespaceIndex = newNode->nodeId.namespaceIndex;

        unsigned long identifier;
        long before, after;
        cds_lfht_count_nodes(ns->ht, &before, &identifier, &after); // current amount of nodes stored
        identifier++;

        newNode->nodeId.identifier.numeric = (UA_UInt32)identifier;
        while(UA_TRUE) {
            hash_t h = hash(&newNode->nodeId);
            result = cds_lfht_add_unique(ns->ht, h, compare, &newNode->nodeId, &entry->htn);
            if(result == &entry->htn)
                break;
            newNode->nodeId.identifier.numeric += (UA_UInt32)(identifier * 2654435761);
        }
    }

    UA_free(node);
    if(inserted)
        *inserted = &entry->node;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, const UA_Node *oldNode, UA_Node *node,
                                   const UA_Node **inserted) {
    /* Get the current version */
    hash_t h = hash(&node->nodeId);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ns->ht, h, compare, &node->nodeId, &iter);
    if(!iter.node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* We try to replace an obsolete version of the node */
    struct nodeEntry *oldEntry = (struct nodeEntry*)iter.node;
    if(&oldEntry->node != oldNode)
        return UA_STATUSCODE_BADINTERNALERROR;
    
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
    memcpy((void*)&newEntry->node, node, nodesize);
    cds_lfht_node_init(&newEntry->htn);

    if(cds_lfht_replace(ns->ht, &iter, h, compare, &node->nodeId, &newEntry->htn) != 0) {
        /* Replacing failed. Maybe the node got replaced just before this thread tried to.*/
        UA_free(newEntry);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
        
    /* If an entry got replaced, mark it as dead. */
    call_rcu(&oldEntry->rcu_head, deleteEntry);
    UA_free(node);

    if(inserted)
        *inserted = &newEntry->node;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid) {
    hash_t h = hash(nodeid);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ns->ht, h, compare, &nodeid, &iter);
    if(!iter.node || cds_lfht_del(ns->ht, iter.node) != 0)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    struct nodeEntry *entry = (struct nodeEntry*)iter.node;
    call_rcu(&entry->rcu_head, deleteEntry);
    return UA_STATUSCODE_GOOD;
}

const UA_Node * UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid) {
    hash_t h = hash(nodeid);
    struct cds_lfht_iter iter;
    cds_lfht_lookup(ns->ht, h, compare, nodeid, &iter);
    struct nodeEntry *found_entry = (struct nodeEntry*)iter.node;
    if(!found_entry)
        return NULL;
    return &found_entry->node;
}

void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor) {
    struct cds_lfht *ht = ns->ht;
    struct cds_lfht_iter iter;
    cds_lfht_first(ht, &iter);
    while(iter.node != NULL) {
        struct nodeEntry *found_entry = (struct nodeEntry*)iter.node;
        visitor(&found_entry->node);
        cds_lfht_next(ht, &iter);
    }
}
