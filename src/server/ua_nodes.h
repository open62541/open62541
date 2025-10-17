
#ifndef UA_NODES_H
#define UA_NODES_H

#include <open62541/server.h>
#include <open62541/plugin/nodestore.h>

_UA_BEGIN_DECLS

const UA_NodeId* UA_Node_Variable_or_VariableType_getDataType(const UA_Node *node);

UA_Int32 UA_Node_Variable_or_VariableType_getValueRank(const UA_Node* node);

size_t UA_Node_Variable_or_VariableType_getArrayDimensionsSize(const UA_Node *node);

const UA_UInt32* UA_Node_Variable_or_VariableType_getArrayDimensions(const UA_Node *node);

UA_ValueSourceType UA_Node_Variable_or_VariableType_getValueSourceType(const UA_Node* node);

const UA_ValueSource *UA_Node_Variable_or_VariableType_getValueSource(const UA_Node* node);


// UA_VariableNode

UA_INLINE const UA_NodeId* UA_VariableNode_getDataType(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getDataType ((const UA_Node *) node);
}

UA_INLINE UA_Int32 UA_VariableNode_getValueRank(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getValueRank((const UA_Node *) node);
}

UA_INLINE size_t UA_VariableNode_getArrayDimensionsSize(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getArrayDimensionsSize((const UA_Node *) node);
}

UA_INLINE const UA_UInt32* UA_VariableNode_getArrayDimensions(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getArrayDimensions((const UA_Node *) node);
}

UA_INLINE UA_ValueSourceType UA_VariableNode_getValueSourceType(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getValueSourceType ((const UA_Node *) node);
}

UA_INLINE const UA_ValueSource *UA_VariableNode_getValueSource(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getValueSource ((const UA_Node *) node);
}

UA_Byte UA_VariableNode_getAccessLevel(const UA_VariableNode* node);

void UA_VariableNode_setAccessLevel(UA_VariableNode* node, UA_Byte accessLevel);

UA_Double UA_VariableNode_getMinimumSamplingInterval(const UA_VariableNode* node);

void UA_VariableNode_setMinimumSamplingInterval(UA_VariableNode* node, UA_Double minimumSamplingInterval);

UA_Boolean UA_VariableNode_getHistorizing(const UA_VariableNode* node);

void UA_VariableNode_setHistorizing(const UA_VariableNode* node, UA_Boolean historizing);

UA_Boolean UA_VariableNode_isDynamic(const UA_VariableNode* node);




_UA_END_DECLS

#endif  // UA_NODES_H
