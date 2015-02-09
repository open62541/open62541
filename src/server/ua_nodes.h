#ifndef UA_NODES_H_
#define UA_NODES_H_

#include "ua_types_generated.h"
#include "ua_types_encoding_binary.h"

#define UA_STANDARD_NODEMEMBERS                 \
    UA_NodeId nodeId;                           \
    UA_NodeClass nodeClass;                     \
    UA_QualifiedName browseName;                \
    UA_LocalizedText displayName;               \
    UA_LocalizedText description;               \
    UA_UInt32 writeMask;                        \
    UA_UInt32 userWriteMask;                    \
    UA_Int32 referencesSize;                    \
    UA_ReferenceNode *references;

typedef struct {
    UA_STANDARD_NODEMEMBERS
} UA_Node;

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Byte eventNotifier;
} UA_ObjectNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ObjectNode)

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
} UA_ObjectTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ObjectTypeNode)

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Variant value;
    UA_NodeId dataType;
    UA_Int32 valueRank;
    UA_Int32 arrayDimensionsSize;
    UA_UInt32 *arrayDimensions;
    UA_Byte accessLevel;
    UA_Byte userAccessLevel;
    UA_Double minimumSamplingInterval;
    UA_Boolean historizing;
} UA_VariableNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_VariableNode)

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Variant value;
    UA_NodeId dataType;
    UA_Int32 valueRank;
    UA_Int32 arrayDimensionsSize;
    UA_UInt32 *arrayDimensions;
    UA_Boolean isAbstract;
} UA_VariableTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_VariableTypeNode)

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
    UA_Boolean symmetric;
    UA_LocalizedText inverseName;
} UA_ReferenceTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ReferenceTypeNode)

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean executable;
    UA_Boolean userExecutable;
} UA_MethodNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_MethodNode)

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean containsNoLoops;
    UA_Byte eventNotifier;
} UA_ViewNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ViewNode)

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
} UA_DataTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_DataTypeNode)

#endif /* UA_NODES_H_ */
