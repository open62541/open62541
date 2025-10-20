
#ifndef UA_NODES_H
#define UA_NODES_H

#include <open62541/server.h>
#include <open62541/plugin/nodestore.h>

_UA_BEGIN_DECLS

UA_StatusCode UA_ValueSource_setInternal (
  UA_ValueSource* valueSource,
  const UA_DataValue *value,
  const UA_ValueSourceNotifications *notifications
);

UA_StatusCode UA_ValueSource_setExternal(
  UA_ValueSource* valueSource,
  UA_DataValue **value,
  const UA_ValueSourceNotifications *notifications
);

UA_StatusCode UA_ValueSource_setCallback(
  UA_ValueSource* valueSource,
  const UA_CallbackValueSource *callbackValueSource
);

const UA_NodeId* UA_Node_Variable_or_VariableType_getDataType(const UA_Node *node);

UA_StatusCode UA_Node_Variable_or_VariableType_setDataType(
  UA_Node *node,
  const UA_NodeId *dataType
);

UA_Int32 UA_Node_Variable_or_VariableType_getValueRank(const UA_Node* node);

void UA_Node_Variable_or_VariableType_setValueRank(UA_Node* node, UA_Int32 valueRank);

size_t UA_Node_Variable_or_VariableType_getArrayDimensionsSize(const UA_Node *node);

const UA_UInt32* UA_Node_Variable_or_VariableType_getArrayDimensions(const UA_Node *node);

UA_StatusCode UA_Node_Variable_or_VariableType_setArrayDimensions(
  UA_Node *node,
  size_t arrayDimensionsSize,
  const UA_UInt32 *arrayDimensions
);

const UA_ValueSource *UA_Node_Variable_or_VariableType_getValueSource(const UA_Node* node);

// UA_VariableNode

static UA_INLINE const UA_NodeId* UA_VariableNode_getDataType(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getDataType ((const UA_Node *) node);
}

static UA_INLINE UA_StatusCode UA_VariableNode_setDataType(
  UA_VariableNode *node,
  const UA_NodeId *dataType
) {
  return UA_Node_Variable_or_VariableType_setDataType(
    (UA_Node *)node,
    dataType
  );
}

static UA_INLINE UA_Int32 UA_VariableNode_getValueRank(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getValueRank((const UA_Node *) node);
}

static UA_INLINE void UA_VariableNode_setValueRank(UA_VariableNode* node, UA_Int32 valueRank) {
  UA_Node_Variable_or_VariableType_setValueRank((UA_Node *) node, valueRank);
}

static UA_INLINE size_t UA_VariableNode_getArrayDimensionsSize(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getArrayDimensionsSize((const UA_Node *) node);
}

static UA_INLINE const UA_UInt32* UA_VariableNode_getArrayDimensions(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getArrayDimensions((const UA_Node *) node);
}

static UA_INLINE UA_StatusCode UA_VariableNode_setArrayDimensions(
  UA_VariableNode *node,
  size_t arrayDimensionsSize,
  const UA_UInt32 *arrayDimensions
) {
  return UA_Node_Variable_or_VariableType_setArrayDimensions(
    (UA_Node *)node,
    arrayDimensionsSize,
    arrayDimensions
  );
}

static UA_INLINE const UA_ValueSource *UA_VariableNode_getValueSource(const UA_VariableNode* node) {
  return UA_Node_Variable_or_VariableType_getValueSource((const UA_Node *) node);
}

UA_Byte UA_VariableNode_getAccessLevel(const UA_VariableNode* node);

void UA_VariableNode_setAccessLevel(UA_VariableNode* node, UA_Byte accessLevel);

UA_Double UA_VariableNode_getMinimumSamplingInterval(const UA_VariableNode* node);

void UA_VariableNode_setMinimumSamplingInterval(UA_VariableNode* node, UA_Double minimumSamplingInterval);

UA_Boolean UA_VariableNode_getHistorizing(const UA_VariableNode* node);

void UA_VariableNode_setHistorizing(UA_VariableNode* node, UA_Boolean historizing);

UA_Boolean UA_VariableNode_isDynamic(const UA_VariableNode* node);

void UA_VariableNode_setDynamic(UA_VariableNode* node, UA_Boolean isDynamic);

static UA_INLINE const UA_NodeId* UA_VariableTypeNode_getDataType(const UA_VariableTypeNode* node) {
  return UA_Node_Variable_or_VariableType_getDataType ((const UA_Node *) node);
}

static UA_INLINE UA_StatusCode UA_VariableTypeNode_setDataType(
  UA_VariableTypeNode *node,
  const UA_NodeId *dataType
) {
  return UA_Node_Variable_or_VariableType_setDataType(
    (UA_Node *)node,
    dataType
  );
}

static UA_INLINE UA_Int32 UA_VariableTypeNode_getValueRank(const UA_VariableTypeNode* node) {
  return UA_Node_Variable_or_VariableType_getValueRank((const UA_Node *) node);
}

static UA_INLINE void UA_VariableTypeNode_setValueRank(UA_VariableTypeNode* node, UA_Int32 valueRank) {
  UA_Node_Variable_or_VariableType_setValueRank((UA_Node *) node, valueRank);
}

static UA_INLINE size_t UA_VariableTypeNode_getArrayDimensionsSize(const UA_VariableTypeNode* node) {
  return UA_Node_Variable_or_VariableType_getArrayDimensionsSize((const UA_Node *) node);
}

static UA_INLINE const UA_UInt32* UA_VariableTypeNode_getArrayDimensions(const UA_VariableTypeNode* node) {
  return UA_Node_Variable_or_VariableType_getArrayDimensions((const UA_Node *) node);
}

static UA_INLINE UA_StatusCode UA_VariableTypeNode_setArrayDimensions(
  UA_VariableTypeNode *node,
  size_t arrayDimensionsSize,
  const UA_UInt32 *arrayDimensions
) {
  return UA_Node_Variable_or_VariableType_setArrayDimensions(
    (UA_Node *)node,
    arrayDimensionsSize,
    arrayDimensions
  );
}

static UA_INLINE const UA_ValueSource *UA_VariableTypeNode_getValueSource(const UA_VariableTypeNode* node) {
  return UA_Node_Variable_or_VariableType_getValueSource((const UA_Node *) node);
}

UA_Boolean UA_VariableTypeNode_getIsAbstract(const UA_VariableTypeNode* node);

void UA_VariableTypeNode_setIsAbstract(UA_VariableTypeNode* node, UA_Boolean isAbstract);

const UA_NodeTypeLifecycle *UA_VariableTypeNode_getNodeTypeLifecycle(const UA_VariableTypeNode* node);

void UA_VariableTypeNode_setNodeTypeLifecycle(UA_VariableTypeNode* node, const UA_NodeTypeLifecycle *lifecycle);

_UA_END_DECLS

#endif  // UA_NODES_H
