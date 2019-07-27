/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julian Grothoff, Torben Deppe, Florian Palm
 */

#ifndef UA_NODESTORE_XML_H_
#define UA_NODESTORE_XML_H_

#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/nodestore_switch.h>

/*
 * Creates xml nodestore and interface to it
*/
UA_StatusCode UA_EXPORT
UA_Nodestore_Xml_Interface_new(UA_NodestoreInterface **store);


UA_Node *
UA_Nodestore_Xml_newNode(void *nsCtx, UA_NodeClass nodeClass);
const UA_Node *
UA_Nodestore_Xml_getNode(void *nsCtx, const UA_NodeId *nodeId);
void
UA_Nodestore_Xml_deleteNode(void *nsCtx, UA_Node *node);
void
UA_Nodestore_Xml_releaseNode(void *nsCtx, const UA_Node *node);
UA_StatusCode
UA_Nodestore_Xml_getNodeCopy(void *nsCtx, const UA_NodeId *nodeId,
                         UA_Node **outNode);
UA_StatusCode
UA_Nodestore_Xml_insertNode(void *nsCtx, UA_Node *node, UA_NodeId *addedNodeId);
UA_StatusCode
UA_Nodestore_Xml_removeNode(void *nsCtx, const UA_NodeId *nodeId);
void
UA_Nodestore_Xml_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
                     void *visitorCtx);
UA_StatusCode
UA_Nodestore_Xml_replaceNode(void *nsCtx, UA_Node *node);
UA_StatusCode
UA_Nodestore_Xml_new(void **nsCtx);
void
UA_Nodestore_Xml_delete(void *nsCtx);

typedef UA_UInt16 (*addNamespaceCb)(UA_Server* server, const char *);

typedef struct {
    int loadTimeMs;
} Statistics;

typedef struct {
    const char *file;
    addNamespaceCb addNamespace;
    const Statistics *stat;
    void *userContext;
} FileHandler;

struct Nodeset;
UA_StatusCode
UA_Nodestore_Xml_load(struct Nodeset *nodeset, const FileHandler *fileHandler);


#endif /* UA_NODESTORE_XML_H_ */