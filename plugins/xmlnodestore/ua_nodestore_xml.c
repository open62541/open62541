#include <open62541/plugin/nodestore_xml.h>
#include "nodesetLoader.h"

UA_Node *
UA_Nodestore_Xml_newNode(void *nsCtx, UA_NodeClass nodeClass)
{
    return NULL;
}
const UA_Node *
UA_Nodestore_Xml_getNode(void *nsCtx, const UA_NodeId *nodeId)
{
    return NULL;
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

static UA_UInt16 nscb(void * userCtxt, const char*uri)
{
    return 2;
}

UA_StatusCode
UA_Nodestore_Xml_new(void **nsCtx)
{
    FileHandler f;
    f.addNamespace = nscb;
    f.userContext = NULL;
    f.file = "~/git/xmlparser/nodesets/testNodeset100nodes.xml";
    loadFile(&f);
    return UA_STATUSCODE_GOOD;
}
void
UA_Nodestore_Xml_delete(void *nsCtx)
{
    return;
}