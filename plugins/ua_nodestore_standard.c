#include "ua_nodestore_standard.h"
#include "../src/ua_util.h"
#include "../src/server/ua_nodestore.h" //TODO make external --> move to plugins?

UA_NodestoreInterface
UA_Nodestore_standard() {
    UA_NodestoreInterface nsi;
    nsi.handle =            UA_NodeStore_new();
    nsi.deleteNodestore =   (UA_NodestoreInterface_deleteNodeStore) UA_NodeStore_delete;
    nsi.newNode =           (UA_NodestoreInterface_newNode)         UA_NodeStore_newNode;
    nsi.deleteNode =        (UA_NodestoreInterface_deleteNode)      UA_NodeStore_deleteNode;
    nsi.insertNode =        (UA_NodestoreInterface_insertNode)      UA_NodeStore_insert;
    nsi.getNode =           (UA_NodestoreInterface_getNode)         UA_NodeStore_get;
    nsi.getNodeCopy =       (UA_NodestoreInterface_getNodeCopy)     UA_NodeStore_getCopy;
    nsi.replaceNode =       (UA_NodestoreInterface_replaceNode)     UA_NodeStore_replace;
    nsi.removeNode =        (UA_NodestoreInterface_removeNode)      UA_NodeStore_remove,
    nsi.iterate =           (UA_NodestoreInterface_iterate)         UA_NodeStore_iterate;
    nsi.releaseNode =       (UA_NodestoreInterface_releaseNode)     UA_NodeStore_release;
    nsi.linkNamespace =     (UA_NodestoreInterface_linkNamespace)   UA_NodeStore_linkNamespace;
    nsi.unlinkNamespace =   (UA_NodestoreInterface_unlinkNamespace) UA_NodeStore_unlinkNamespace;
    return nsi;
}

void
UA_Nodestore_standard_delete(UA_NodestoreInterface * nodestoreInterface){
    //No RCU Lock available, defined in server_interal
    //UA_RCU_LOCK();
    nodestoreInterface->deleteNodestore(nodestoreInterface->handle, UA_UINT16_MAX);
    //UA_RCU_UNLOCK();
    UA_free(nodestoreInterface->handle);
}
