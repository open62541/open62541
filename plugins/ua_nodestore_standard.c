#include "ua_nodestore_standard.h"
#include "../src/ua_util.h"
#include "../src/server/ua_nodestore.h" //TODO make external --> move to plugins?

UA_NodestoreInterface
UA_Nodestore_standard() {
    UA_NodestoreInterface nsi;
    nsi.handle =        UA_NodeStore_new();
    nsi.deleteNodeStore =        (UA_NodestoreInterface_delete)      UA_NodeStore_delete;
    nsi.newNode =       (UA_NodestoreInterface_newNode)     UA_NodeStore_newNode;
    nsi.deleteNode =    (UA_NodestoreInterface_deleteNode)  UA_NodeStore_deleteNode;
    nsi.insert =        (UA_NodestoreInterface_insert)      UA_NodeStore_insert;
    nsi.get =           (UA_NodestoreInterface_get)         UA_NodeStore_get;
    nsi.getCopy =       (UA_NodestoreInterface_getCopy)     UA_NodeStore_getCopy;
    nsi.replace =       (UA_NodestoreInterface_replace)     UA_NodeStore_replace;
    nsi.remove =        (UA_NodestoreInterface_remove)      UA_NodeStore_remove,
    nsi.iterate =       (UA_NodestoreInterface_iterate)     UA_NodeStore_iterate;
    return nsi;
}

void
UA_Nodestore_standard_delete(UA_NodestoreInterface * nodestoreInterface){
    //No RCU Lock available, defined in server_interal
    //UA_RCU_LOCK();
    //nodestoreInterface->deleteNodeStore(nodestoreInterface->handle);
    //UA_RCU_UNLOCK();
    UA_free(nodestoreInterface->handle);
}
