#ifndef UA_NODES_H_
#define UA_NODES_H_

#include "ua_types_generated.h"

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

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
} UA_ObjectTypeNode;

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

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Variant value;
    UA_NodeId dataType;
    UA_Int32 valueRank;
    UA_Int32 arrayDimensionsSize;
    UA_UInt32 *arrayDimensions;
    UA_Boolean isAbstract;
} UA_VariableTypeNode;

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
    UA_Boolean symmetric;
    UA_LocalizedText inverseName;
} UA_ReferenceTypeNode;

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean executable;
    UA_Boolean userExecutable;
} UA_MethodNode;

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean containsNoLoops;
    UA_Byte eventNotifier;
} UA_ViewNode;

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
} UA_DataTypeNode;

#endif /* UA_NODES_H_ */
