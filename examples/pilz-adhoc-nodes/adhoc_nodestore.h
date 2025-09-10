#include <open62541/util.h>
#include <open62541/plugin/nodestore_default.h>

UA_Nodestore * UA_Nodestore_PilzAdHoc(void);

UA_Boolean isBackendNode(const UA_NodeId *nodeId);
void collectParentInfo(UA_Node *node);
void collectChildsInfo(UA_Node *node);
void collectNodeAttributes(UA_Node *node);
