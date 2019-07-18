#include <open62541/plugin/nodestore_xml.h>
#include "nodesetLoader.h"
#include "nodeset.h"

UA_Node *
UA_Nodestore_Xml_newNode(void *nsCtx, UA_NodeClass nodeClass)
{
    return NULL;
}
const UA_Node *
UA_Nodestore_Xml_getNode(void *nsCtx, const UA_NodeId *nodeId)
{
    return Nodeset_getNode(nodeId);
}
void
UA_Nodestore_Xml_deleteNode(void *nsCtx, UA_Node *node)
{
    return;
}
void
UA_Nodestore_Xml_releaseNode(void *nsCtx, const UA_Node *node)
{
    return;
}
UA_StatusCode
UA_Nodestore_Xml_getNodeCopy(void *nsCtx, const UA_NodeId *nodeId,
                         UA_Node **outNode)
{
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}
UA_StatusCode
UA_Nodestore_Xml_insertNode(void *nsCtx, UA_Node *node, UA_NodeId *addedNodeId)
{
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}
UA_StatusCode
UA_Nodestore_Xml_removeNode(void *nsCtx, const UA_NodeId *nodeId)
{
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

void
UA_Nodestore_Xml_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
                     void *visitorCtx)
{
    return;
}
UA_StatusCode
UA_Nodestore_Xml_replaceNode(void *nsCtx, UA_Node *node)
{
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

UA_StatusCode
UA_Nodestore_Xml_new(void **nsCtx, const FileHandler* f)
{
    *nsCtx = Nodeset_new(f->addNamespace);
    return UA_STATUSCODE_GOOD;
}

void UA_Nodestore_Xml_load(void* nsCtx, const FileHandler* f)
{    
    loadXmlFile((Nodeset*) nsCtx, f);
}

void
UA_Nodestore_Xml_delete(void *nsCtx)
{
    return;
}