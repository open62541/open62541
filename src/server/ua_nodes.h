#ifndef UA_NODES_H_
#define UA_NODES_H_

#include "ua_server.h"
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

/**************/
/* ObjectNode */
/**************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Byte eventNotifier;
} UA_ObjectNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ObjectNode)

/******************/
/* ObjectTypeNode */
/******************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
} UA_ObjectTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ObjectTypeNode)

typedef enum {
    UA_VALUESOURCE_VARIANT,
    UA_VALUESOURCE_DATASOURCE
} UA_ValueSource;

/****************/
/* VariableNode */
/****************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Int32 valueRank; /**< n >= 1: the value is an array with the specified number of dimensions.
                             n = 0: the value is an array with one or more dimensions.
                             n = -1: the value is a scalar.
                             n = -2: the value can be a scalar or an array with any number of dimensions.
                             n = -3:  the value can be a scalar or a one dimensional array. */
    UA_ValueSource valueSource;
    union {
        UA_Variant variant;
        UA_DataSource dataSource;
    } value;
    /* <--- similar to variabletypenodes up to there--->*/
    UA_Byte accessLevel;
    UA_Byte userAccessLevel;
    UA_Double minimumSamplingInterval;
    UA_Boolean historizing;
} UA_VariableNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_VariableNode)
/** Make a copy but leave out the references and the variable */
UA_StatusCode UA_VariableNode_copyWithoutRefsAndVariable(const UA_VariableNode *src, UA_VariableNode *dst);

/********************/
/* VariableTypeNode */
/********************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Int32 valueRank;
    UA_ValueSource valueSource;
    union {
        UA_Variant variant;
        UA_DataSource dataSource;
    } value;
    /* <--- similar to variablenodes up to there--->*/
    UA_Boolean isAbstract;
} UA_VariableTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_VariableTypeNode)

/*********************/
/* ReferenceTypeNode */
/*********************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
    UA_Boolean symmetric;
    UA_LocalizedText inverseName;
} UA_ReferenceTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ReferenceTypeNode)

/***********************/
/* ReferenceMethodNode */
/***********************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean executable;
    UA_Boolean userExecutable;
#ifdef ENABLE_METHODCALLS
    UA_MethodCallback attachedMethod;
#endif
} UA_MethodNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_MethodNode)

/************/
/* ViewNode */
/************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean containsNoLoops;
    UA_Byte eventNotifier;
} UA_ViewNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_ViewNode)

/****************/
/* DataTypeNode */
/****************/

typedef struct {
    UA_STANDARD_NODEMEMBERS
    UA_Boolean isAbstract;
} UA_DataTypeNode;
UA_TYPE_HANDLING_FUNCTIONS(UA_DataTypeNode)

#endif /* UA_NODES_H_ */
