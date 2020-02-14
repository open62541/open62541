/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2014-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2020 (c) Kalycito Infotech Pvt Ltd (Author: Jayanth Velusamy)
 *
 *    This nodestore contains the copy of zipTree to hold the MINIMAL nodes
 */


#include <open62541/plugin/nodestore_default.h>
#include <open62541/plugin/log_stdout.h>
#include "ziptree.h"
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/util.h>

#ifdef __linux__
#include <stdio.h>
#include <sys/mman.h>
#endif

#if defined UA_ENABLE_ENCODE_AND_DUMP || defined UA_ENABLE_USE_ENCODED_NODES
static UA_StatusCode
commonVariableAttributeEncode(const UA_VariableNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_encodeBinary(&node->dataType, &bufPos, bufEnd);
    retval |= UA_Int32_encodeBinary(&node->valueRank, &bufPos, bufEnd);
    retval |= UA_UInt64_encodeBinary(&node->arrayDimensionsSize, &bufPos, bufEnd);
    if(node->arrayDimensionsSize) {
        retval |= UA_UInt32_encodeBinary(node->arrayDimensions, &bufPos, bufEnd);
    }
    retval |= UA_UInt32_encodeBinary((const UA_UInt32*)&node->valueSource, &bufPos, bufEnd);
    UA_DataValue v2 = node->value.data.value;
    retval |= UA_DataValue_encodeBinary(&v2, &bufPos, bufEnd);
    return retval;
}

static UA_StatusCode
objectNodeEncode(const UA_ObjectNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    return UA_Byte_encodeBinary(&node->eventNotifier, &bufPos, bufEnd);
}

static UA_StatusCode
variableNodeEncode(const UA_VariableNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Byte_encodeBinary(&node->accessLevel, &bufPos, bufEnd);
    retval |= UA_Double_encodeBinary(&node->minimumSamplingInterval, &bufPos, bufEnd);
    retval |= UA_Boolean_encodeBinary(&node->historizing, &bufPos, bufEnd);
    retval |= commonVariableAttributeEncode(node, bufPos, bufEnd);
    return retval;
}

static UA_StatusCode
methodNodeEncode(const UA_MethodNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    return UA_Boolean_encodeBinary(&node->executable, &bufPos, bufEnd);
}

static UA_StatusCode
objectTypeNodeEncode(const UA_ObjectTypeNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    return UA_Boolean_encodeBinary(&node->isAbstract, &bufPos, bufEnd);
}

static UA_StatusCode
variableTypeNodeEncode(const UA_VariableTypeNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Boolean_encodeBinary(&node->isAbstract, &bufPos, bufEnd);
    retval |= commonVariableAttributeEncode((const UA_VariableNode*)node, bufPos, bufEnd);
    return retval;
}

static UA_StatusCode
ReferenceTypeNodeEncode(const UA_ReferenceTypeNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Boolean_encodeBinary(&node->isAbstract, &bufPos, bufEnd);
    retval |= UA_Boolean_encodeBinary(&node->symmetric, &bufPos, bufEnd);
    retval |= UA_LocalizedText_encodeBinary(&node->inverseName, &bufPos, bufEnd);
    return retval;
}

static UA_StatusCode
dataTypeNodeEncode(const UA_DataTypeNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    return UA_Boolean_encodeBinary(&node->isAbstract, &bufPos, bufEnd);
}

static UA_StatusCode
viewNodeEncode(const UA_ViewNode *node, UA_Byte *bufPos, const UA_Byte *bufEnd) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Byte_encodeBinary(&node->eventNotifier, &bufPos, bufEnd);
    retval |= UA_Boolean_encodeBinary(&node->containsNoLoops, &bufPos, bufEnd);
    return retval;
}

static UA_StatusCode
UA_NodeReferenceKind_encodeBinary(const UA_NodeReferenceKind *references,
                                  UA_Byte **bufPos, const UA_Byte **bufEnd) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_encodeBinary(&references->referenceTypeId, bufPos, *bufEnd);
    retval |= UA_Boolean_encodeBinary(&references->isInverse, bufPos, *bufEnd);
    retval |= UA_UInt64_encodeBinary((const UA_UInt64 *)&references->refTargetsSize, bufPos, *bufEnd);
    for(size_t i = 0; i < references->refTargetsSize; i++) {
        UA_ReferenceTarget *refTarget = &references->refTargets[i];
        retval |= UA_UInt32_encodeBinary(&refTarget->targetHash, bufPos, *bufEnd);
        retval |= UA_ExpandedNodeId_encodeBinary(&refTarget->target, bufPos, *bufEnd);
    }
    return retval;
}

UA_StatusCode
UA_Node_encode(const UA_Node *node, UA_ByteString *new_valueEncoding) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_Byte *bufPos = new_valueEncoding->data;
    const UA_Byte *bufEnd = &new_valueEncoding->data[new_valueEncoding->length];
    retval |= UA_NodeClass_encodeBinary(&node->nodeClass, &bufPos, bufEnd);
    retval |= UA_NodeId_encodeBinary(&node->nodeId, &bufPos, bufEnd);
    retval |= UA_QualifiedName_encodeBinary(&node->browseName, &bufPos, bufEnd);
    retval |= UA_LocalizedText_encodeBinary(&node->displayName, &bufPos, bufEnd);
    retval |= UA_LocalizedText_encodeBinary(&node->description, &bufPos, bufEnd);
    retval |= UA_UInt32_encodeBinary(&node->writeMask, &bufPos, bufEnd);
    retval |= UA_UInt64_encodeBinary((const UA_UInt64 *)&node->referencesSize, &bufPos, bufEnd);
    for (size_t i = 0; i < node->referencesSize; i++) {
        retval |= UA_NodeReferenceKind_encodeBinary(&node->references[i], &bufPos, &bufEnd);
    }

    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        retval |= objectNodeEncode((const UA_ObjectNode*)node, bufPos, bufEnd);
        break;
    case UA_NODECLASS_VARIABLE:
        retval |= variableNodeEncode((const UA_VariableNode*)node, bufPos, bufEnd);
        break;
    case UA_NODECLASS_METHOD:
        retval |= methodNodeEncode((const UA_MethodNode*)node, bufPos, bufEnd);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        retval |= objectTypeNodeEncode((const UA_ObjectTypeNode*)node, bufPos, bufEnd);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        retval |= variableTypeNodeEncode((const UA_VariableTypeNode*)node, bufPos, bufEnd);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        retval |= ReferenceTypeNodeEncode((const UA_ReferenceTypeNode*)node, bufPos, bufEnd);
        break;
    case UA_NODECLASS_DATATYPE:
        retval |= dataTypeNodeEncode((const UA_DataTypeNode*)node, bufPos, bufEnd);
        break;
    case UA_NODECLASS_VIEW:
        retval |= viewNodeEncode((const UA_ViewNode*)node, bufPos, bufEnd);
        break;
    default:
        break;
    }

    return retval;
}
#endif

#ifdef UA_ENABLE_USE_ENCODED_NODES

#define MAX_ROW_LENGTH         30  // Maximum length of a row in lookup table

#ifndef UA_ENABLE_IMMUTABLE_NODES
#error The ROM-based Nodestore requires nodes to be replaced on write
#endif

static UA_StatusCode
objectNodeDecode(const UA_ByteString *src, size_t *offset, UA_ObjectNode* objectNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Byte_decodeBinary(src, offset, &objectNode->eventNotifier);
    return retval;
}

static UA_StatusCode
variableNodeDecode(const UA_ByteString *src, size_t *offset, UA_VariableNode* variableNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Byte_decodeBinary(src, offset, &variableNode->accessLevel);
    retval |= UA_Double_decodeBinary(src, offset, &variableNode->minimumSamplingInterval);
    retval |= UA_Boolean_decodeBinary(src, offset, &variableNode->historizing);
    retval |= UA_NodeId_decodeBinary(src, offset, &variableNode->dataType);
    retval |= UA_Int32_decodeBinary(src, offset, &variableNode->valueRank);
    retval |= UA_UInt64_decodeBinary(src, offset, &variableNode->arrayDimensionsSize);
    if(variableNode->arrayDimensionsSize) {
        variableNode->arrayDimensions = (UA_UInt32 *)UA_calloc(variableNode->arrayDimensionsSize, sizeof(UA_UInt32));
        if(!variableNode->arrayDimensions) {
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        retval |= UA_UInt32_decodeBinary(src, offset, variableNode->arrayDimensions);
    }
    retval |= UA_UInt32_decodeBinary(src, offset, (UA_UInt32*)&variableNode->valueSource);
    retval |= UA_DataValue_decodeBinary(src, offset, &variableNode->value.data.value);
    return retval;
}

static UA_StatusCode
methodNodeDecode(const UA_ByteString *src, size_t *offset, UA_MethodNode* methodNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Boolean_decodeBinary(src, offset, &methodNode->executable);
    return retval;
}

static UA_StatusCode
objectTypeNodeDecode(const UA_ByteString *src, size_t *offset, UA_ObjectTypeNode* objTypeNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Boolean_decodeBinary(src, offset, &objTypeNode->isAbstract);
    return retval;
}

static UA_StatusCode
variableTypeNodeDecode(const UA_ByteString *src, size_t *offset, UA_VariableTypeNode* varTypeNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Boolean_decodeBinary(src, offset, &varTypeNode->isAbstract);
    retval |= UA_NodeId_decodeBinary(src, offset, &varTypeNode->dataType);
    retval |= UA_Int32_decodeBinary(src, offset, &varTypeNode->valueRank);
    retval |= UA_UInt64_decodeBinary(src, offset, &varTypeNode->arrayDimensionsSize);
    if(varTypeNode->arrayDimensionsSize) {
        retval |= UA_UInt32_decodeBinary(src, offset, &varTypeNode->arrayDimensions[0]);
    }
    retval |= UA_UInt32_decodeBinary(src, offset, (UA_UInt32*)&varTypeNode->valueSource);
    retval |= UA_DataValue_decodeBinary(src, offset, &varTypeNode->value.data.value);

    return retval;
}

static UA_StatusCode
ReferenceTypeNodeDecode(const UA_ByteString *src, size_t *offset, UA_ReferenceTypeNode* refTypeNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Boolean_decodeBinary(src, offset, &refTypeNode->isAbstract);
    retval |= UA_Boolean_decodeBinary(src, offset, &refTypeNode->symmetric);
    retval |= UA_LocalizedText_decodeBinary(src, offset, &refTypeNode->inverseName);
    return retval;
}

static UA_StatusCode
dataTypeNodeDecode(const UA_ByteString *src, size_t *offset, UA_DataTypeNode* dataTypeNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Boolean_decodeBinary(src, offset, &dataTypeNode->isAbstract);
    return retval;
}

static UA_StatusCode
viewNodeDecode(const UA_ByteString *src, size_t *offset, UA_ViewNode* viewNode) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Byte_decodeBinary(src, offset, &viewNode->eventNotifier);
    retval |= UA_Boolean_decodeBinary(src, offset, &viewNode->containsNoLoops);
    return retval;
}

static UA_UInt32
countNodes(const char *const path) {
    int ch;
    UA_UInt32 nodeCount = 0; // To count the number of nodes
    FILE *fpLookuptable;
    fpLookuptable = fopen(path, "r");
    if(!fpLookuptable) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The opening of file lookupTable.bin failed");
    }
    while((ch = fgetc(fpLookuptable)) != EOF) {
        if(ch == '\n') {
            nodeCount++;
        }
    }
    fclose(fpLookuptable);
    return nodeCount;
}

static UA_StatusCode
UA_Read_Encoded_Binary(UA_ByteString *encodedBin, const char *const path) {
    /* Allocate and initialize the context */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    FILE *fpEncoded = fopen(path, "r");
    if(!fpEncoded) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The opening of file encodedNode.bin failed");
        retval |= UA_STATUSCODE_BADINTERNALERROR;
        return retval;
    }
    fseek(fpEncoded, 0L, SEEK_END);
    size_t sizeOfEncodedBin = (size_t)ftell(fpEncoded);
    void *mmapped = mmap(NULL, sizeOfEncodedBin, PROT_READ, MAP_PRIVATE, fileno(fpEncoded), 0);
    encodedBin->data = (UA_Byte*)mmapped;
    encodedBin->length = sizeOfEncodedBin;
    fclose(fpEncoded);
    return retval;
}

/* container_of */
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))

struct NodeEntry;
typedef struct NodeEntry NodeEntry;

struct NodeEntry {
    ZIP_ENTRY(NodeEntry) zipfields;
    UA_UInt32 nodeIdHash;
    UA_UInt16 refCount; /* How many consumers have a reference to the node? */
    UA_Boolean deleted; /* Node was marked as deleted and can be deleted when refCount == 0 */
    NodeEntry *orig;    /* If a copy is made to replace a node, track that we
                         * replace only the node from which the copy was made.
                         * Important for concurrent operations. */
    UA_NodeId nodeId; /* This is actually a UA_Node that also starts with a NodeId */
};

struct LtEntry;
typedef struct LtEntry LtEntry;

struct LtEntry {
    ZIP_ENTRY(LtEntry) zipfieldsLt;
    lookUpTable ltRead;
};

/* Absolute ordering for NodeIds */
static enum ZIP_CMP
cmpNodeId(const void *a, const void *b) {
    const NodeEntry *aa = (const NodeEntry*)a;
    const NodeEntry *bb = (const NodeEntry*)b;

    /* Compare hash */
    if(aa->nodeIdHash < bb->nodeIdHash)
        return ZIP_CMP_LESS;
    if(aa->nodeIdHash > bb->nodeIdHash)
        return ZIP_CMP_MORE;

    /* Compore nodes in detail */
    return (enum ZIP_CMP)UA_NodeId_order(&aa->nodeId, &bb->nodeId);
}

/* Absolute ordering for NodeIds */
static enum ZIP_CMP
cmpNodeIdLt(const void *a, const void *b) {
    const LtEntry *aa = (const LtEntry*)a;
    const LtEntry *bb = (const LtEntry*)b;

    /* Compare nodes in detail */
    return (enum ZIP_CMP)UA_NodeId_order(&aa->ltRead.nodeId, &bb->ltRead.nodeId);
}

ZIP_HEAD(NodeTreeBin, NodeEntry);
typedef struct NodeTreeBin NodeTreeBin;

ZIP_HEAD(NodeTreeLt, LtEntry);
typedef struct NodeTreeLt NodeTreeLt;

typedef struct {
    NodeTreeBin root;
    NodeTreeLt ltRoot;
    LtEntry *lte;
    UA_UInt32 ltSize;
    UA_ByteString encodeBin;
} ZipContext;

ZIP_PROTTYPE(NodeTreeBin, NodeEntry, NodeEntry)
ZIP_IMPL(NodeTreeBin, NodeEntry, zipfields, NodeEntry, zipfields, cmpNodeId)

ZIP_PROTTYPE(NodeTreeLt, LtEntry, LtEntry)
ZIP_IMPL(NodeTreeLt, LtEntry, zipfieldsLt, LtEntry, zipfieldsLt, cmpNodeIdLt)

static NodeEntry *
newEntry(UA_NodeClass nodeClass) {
    size_t size = sizeof(NodeEntry) - sizeof(UA_NodeId);
    switch(nodeClass) {
    case UA_NODECLASS_OBJECT:
        size += sizeof(UA_ObjectNode);
        break;
    case UA_NODECLASS_VARIABLE:
        size += sizeof(UA_VariableNode);
        break;
    case UA_NODECLASS_METHOD:
        size += sizeof(UA_MethodNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        size += sizeof(UA_ObjectTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        size += sizeof(UA_VariableTypeNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        size += sizeof(UA_ReferenceTypeNode);
        break;
    case UA_NODECLASS_DATATYPE:
        size += sizeof(UA_DataTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        size += sizeof(UA_ViewNode);
        break;
    default:
        return NULL;
    }
    NodeEntry *entry = (NodeEntry*)UA_calloc(1, size);
    if(!entry)
        return NULL;
    UA_Node *node = (UA_Node*)&entry->nodeId;
    node->nodeClass = nodeClass;
    return entry;
}

static void
deleteEntry(NodeEntry *entry) {
    UA_Node_clear((UA_Node*)&entry->nodeId);
    UA_free(entry);
}

static void
cleanupEntry(NodeEntry *entry) {
    if(entry->deleted && entry->refCount == 0)
        deleteEntry(entry);
}

/***********************/
/* Interface functions */
/***********************/

/* Not yet inserted into the ZipContext */
static UA_Node *
zipNsNewNode(void *nsCtx, UA_NodeClass nodeClass) {
    NodeEntry *entry = newEntry(nodeClass);
    if(!entry)
        return NULL;
    return (UA_Node*)&entry->nodeId;
}

/* Not yet inserted into the ZipContext */
static void
zipNsDeleteNode(void *nsCtx, UA_Node *node) {
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    entry->deleted = true;
    deleteEntry(entry);
}

static void
zipNsReleaseNode(void *nsCtx, const UA_Node *node) {
    if(!node)
        return;
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    UA_assert(entry->refCount > 0);
    --entry->refCount;
    cleanupEntry(entry);
}

static UA_StatusCode
parseGuid(char arg[], UA_Guid *guid) {
    guid = UA_Guid_new();
    guid->data1 = (UA_UInt32)strtoull(arg, NULL, 16);
    guid->data2 = (UA_UInt16)strtoull(&arg[9], NULL, 16);
    guid->data3 = (UA_UInt16)strtoull(&arg[14], NULL, 16);
    UA_Int16 data4_1 = (UA_Int16)strtoull(&arg[19], NULL, 16);
    guid->data4[0] = (UA_Byte)(data4_1 >> 8);
    guid->data4[1] = (UA_Byte)data4_1;
    UA_Int64 data4_2 = (UA_Int64)strtoull(&arg[24], NULL, 16);
    guid->data4[2] = (UA_Byte)(data4_2 >> 40);
    guid->data4[3] = (UA_Byte)(data4_2 >> 32);
    guid->data4[4] = (UA_Byte)(data4_2 >> 24);
    guid->data4[5] = (UA_Byte)(data4_2 >> 16);
    guid->data4[6] = (UA_Byte)(data4_2 >> 8);
    guid->data4[7] = (UA_Byte)data4_2;
    return UA_STATUSCODE_GOOD;
}


/* Parse NodeId from String */
static UA_StatusCode
parseNodeId(UA_String str, lookUpTable *ltRow) {

    UA_NodeId *id = &ltRow->nodeId;
    /* zero-terminated string */
    UA_STACKARRAY(char, s, str.length + 1);
    memcpy(s, str.data, str.length);
    s[str.length] = 0;

    UA_NodeId_init(id);

    /* Int identifier */
    if(sscanf(s, "ns=%hu;i=%u %lu %lu", &id->namespaceIndex, &id->identifier.numeric,
            &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "ns=%hu,i=%u %lu %lu", &id->namespaceIndex, &id->identifier.numeric,
               &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "i=%u", &id->identifier.numeric) == 1) {
        id->identifierType = UA_NODEIDTYPE_NUMERIC;
        return UA_STATUSCODE_GOOD;
    }

    /* String identifier */
    UA_STACKARRAY(char, sid, str.length);
    if(sscanf(s, "ns=%hu;s=%s %lu %lu", &id->namespaceIndex, sid,
            &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "ns=%hu,s=%s %lu %lu", &id->namespaceIndex, sid,
               &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "s=%s", sid) == 1) {
        id->identifierType = UA_NODEIDTYPE_STRING;
        id->identifier.string = UA_String_fromChars(sid);
        return UA_STATUSCODE_GOOD;
    }

    /* ByteString idenifier */
    if(sscanf(s, "ns=%hu;b=%s %lu %lu", &id->namespaceIndex, sid,
            &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "ns=%hu,b=%s %lu %lu", &id->namespaceIndex, sid,
               &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "b=%s", sid) == 1) {
        id->identifierType = UA_NODEIDTYPE_BYTESTRING;
        id->identifier.string = UA_String_fromChars(sid);
        return UA_STATUSCODE_GOOD;
    }

    /* Guid idenifier */
    if(sscanf(s, "ns=%hu;g=%s %lu %lu", &id->namespaceIndex, sid,
            &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "ns=%hu,g=%s %lu %lu", &id->namespaceIndex, sid,
               &ltRow->nodePosition, &ltRow->nodeSize) == 4 ||
       sscanf(s, "g=%s", sid) == 1) {
        UA_StatusCode retval = parseGuid(sid, &id->identifier.guid);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        id->identifierType = UA_NODEIDTYPE_GUID;
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
UA_Read_LookUpTable(void *nsCtx, const char* const path){
    int ch;
    int length = 0;
    int row = 0;
    char ltRow[MAX_ROW_LENGTH] = {0};
    UA_String str = UA_STRING_NULL;
    ZipContext *ns = (ZipContext *)nsCtx;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    ns->ltSize = countNodes(path);
    ns->lte = (LtEntry*)UA_calloc(ns->ltSize, sizeof(LtEntry));
    if(!ns->lte) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    FILE *fpLookuptable  = fopen(path, "r");
    if(!fpLookuptable) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The opening of file lookupTable.bin failed");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    while((ch = fgetc(fpLookuptable)) != EOF) {
        switch(ch) {
        case '\n':
            ltRow[length] = '\0';
            str = UA_String_fromChars(ltRow);
            retval |= parseNodeId(str, &ns->lte[row].ltRead);//separate lookuptable content from each row and populate
            UA_free(str.data);
            ZIP_INSERT(NodeTreeLt, &ns->ltRoot, &ns->lte[row], ZIP_FFS32(UA_UInt32_random()));
            for(int i = 0; i < MAX_ROW_LENGTH; i++) {
                ltRow[i]= 0;
            }
            length = 0;
            row++;
            break;
        default:
            ltRow[length] = (char)ch; // Read each character until a newline is found
            length++;
            break;
        }
    }
    fclose(fpLookuptable);
    return retval;
}

UA_Node*
decodeNode(void *ctx, UA_ByteString encodedBin, size_t offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_NodeClass nodeClass;
    retval |= UA_NodeClass_decodeBinary(&encodedBin, &offset, &nodeClass);
    UA_Node* node = zipNsNewNode(ctx, nodeClass);
    if(!node) {
        return NULL;
    }

    node->context = ctx;
    node->nodeClass = nodeClass;
    retval |= UA_NodeId_decodeBinary(&encodedBin, &offset, &node->nodeId);
    retval |= UA_QualifiedName_decodeBinary(&encodedBin, &offset, &node->browseName);
    retval |= UA_LocalizedText_decodeBinary(&encodedBin, &offset, &node->displayName);
    retval |= UA_LocalizedText_decodeBinary(&encodedBin, &offset, &node->description);
    retval |= UA_UInt32_decodeBinary(&encodedBin, &offset, &node->writeMask);
    retval |= UA_UInt64_decodeBinary(&encodedBin, &offset, (UA_UInt64 *)&node->referencesSize);

    node->references = (UA_NodeReferenceKind*) UA_calloc(node->referencesSize, sizeof(UA_NodeReferenceKind));
    if(!node->references) {
        return NULL;
    }
    for (size_t i = 0; i < node->referencesSize; i++) {
        retval |= UA_NodeId_decodeBinary(&encodedBin, &offset, &node->references[i].referenceTypeId);
        retval |= UA_Boolean_decodeBinary(&encodedBin, &offset, &node->references[i].isInverse);

        size_t refTargetsSize;
        retval |= UA_UInt64_decodeBinary(&encodedBin, &offset, (UA_UInt64 *)&refTargetsSize);
        node->references[i].refTargetsSize = refTargetsSize;
        node->references[i].refTargets = (UA_ReferenceTarget*)UA_calloc(node->references[i].refTargetsSize,
                                                 sizeof(UA_ReferenceTarget));
        if(!node->references[i].refTargets ) {
            return NULL;
        }
        for (size_t j = 0; j < refTargetsSize; j++) {
            retval |= UA_UInt32_decodeBinary(&encodedBin, &offset, &node->references[i].refTargets[j].targetHash);
            retval |= UA_ExpandedNodeId_decodeBinary(&encodedBin, &offset, &node->references[i].refTargets[j].target);
        }
    }

    switch(nodeClass) {
    case UA_NODECLASS_OBJECT:
           retval |= objectNodeDecode(&encodedBin, &offset, (UA_ObjectNode*) node);
        break;
    case UA_NODECLASS_VARIABLE:
        retval |= variableNodeDecode(&encodedBin, &offset, (UA_VariableNode*) node);
        break;
    case UA_NODECLASS_METHOD:
           retval |= methodNodeDecode(&encodedBin, &offset, (UA_MethodNode*) node);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        retval |= objectTypeNodeDecode(&encodedBin, &offset, (UA_ObjectTypeNode*) node);
        break;
    case UA_NODECLASS_VARIABLETYPE:
           retval |= variableTypeNodeDecode(&encodedBin, &offset, (UA_VariableTypeNode*) node);
        break;
    case UA_NODECLASS_REFERENCETYPE:
          retval |= ReferenceTypeNodeDecode(&encodedBin, &offset, (UA_ReferenceTypeNode*) node);
        break;
    case UA_NODECLASS_DATATYPE:
          retval |= dataTypeNodeDecode(&encodedBin, &offset, (UA_DataTypeNode*) node);
        break;
    case UA_NODECLASS_VIEW:
          retval |= viewNodeDecode(&encodedBin, &offset, (UA_ViewNode*) node);
        break;
    default:
        break;
    }

    if(retval != UA_STATUSCODE_GOOD) {
           UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The decodeNode failed with error : %s",
                                          UA_StatusCode_name(retval));
    }

    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    ++entry->refCount;
    entry->deleted = true; // remove the decoded node from the memory when release is called!

    return node;
}

static const UA_Node *
zipNsGetNode(void *nsCtx, const UA_NodeId *nodeId) {
    ZipContext *ns = (ZipContext*)nsCtx;
    NodeEntry dummy;
    dummy.nodeIdHash = UA_NodeId_hash(nodeId);
    dummy.nodeId = *nodeId;
    NodeEntry *entry = ZIP_FIND(NodeTreeBin, &ns->root, &dummy);
    if(entry) {
        ++entry->refCount;
        return (const UA_Node*)&entry->nodeId;
    }

    LtEntry ltDummy;
    ltDummy.ltRead.nodeId = *nodeId;
    LtEntry *lte = ZIP_FIND(NodeTreeLt, &ns->ltRoot, &ltDummy);
    if(lte) {
        return decodeNode(nsCtx, ns->encodeBin, lte->ltRead.nodePosition);
    }
    return NULL;
}

static UA_StatusCode
zipNsGetNodeCopy(void *nsCtx, const UA_NodeId *nodeId,
                         UA_Node **outNode) {
    /* Find the node */
    const UA_Node *node = zipNsGetNode(nsCtx, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Create the new entry */
    NodeEntry *ne = newEntry(node->nodeClass);
    if(!ne) {
        zipNsReleaseNode(nsCtx, node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Copy the node content */
    UA_Node *nnode = (UA_Node*)&ne->nodeId;
    UA_StatusCode retval = UA_Node_copy(node, nnode);
    zipNsReleaseNode(nsCtx, node);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteEntry(ne);
        return retval;
    }

    ne->orig = container_of(node, NodeEntry, nodeId);
    *outNode = nnode;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
zipNsInsertNode(void *nsCtx, UA_Node *node, UA_NodeId *addedNodeId) {
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    ZipContext *ns = (ZipContext*)nsCtx;

    /* Ensure that the NodeId is unique */
    NodeEntry dummy;
    dummy.nodeId = node->nodeId;
    if(node->nodeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
       node->nodeId.identifier.numeric == 0) {
        do { /* Create a random nodeid until we find an unoccupied id */
            node->nodeId.identifier.numeric = UA_UInt32_random();
            dummy.nodeId.identifier.numeric = node->nodeId.identifier.numeric;
            dummy.nodeIdHash = UA_NodeId_hash(&node->nodeId);
        } while(ZIP_FIND(NodeTreeBin, &ns->root, &dummy));
    } else {
        dummy.nodeIdHash = UA_NodeId_hash(&node->nodeId);
        if(ZIP_FIND(NodeTreeBin, &ns->root, &dummy)) { /* The nodeid exists */
            deleteEntry(entry);
            return UA_STATUSCODE_BADNODEIDEXISTS;
        }
    }

    /* Copy the NodeId */
    if(addedNodeId) {
        UA_StatusCode retval = UA_NodeId_copy(&node->nodeId, addedNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            deleteEntry(entry);
            return retval;
        }
    }

    /* Insert the node */
    entry->nodeIdHash = dummy.nodeIdHash;
    ZIP_INSERT(NodeTreeBin, &ns->root, entry, ZIP_FFS32(UA_UInt32_random()));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
zipNsReplaceNode(void *nsCtx, UA_Node *node) {
    ZipContext *ns = (ZipContext*)nsCtx;
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);

    /* Find the node */
    const UA_Node *oldNode = zipNsGetNode(nsCtx, &node->nodeId);
    if(!oldNode) {
        deleteEntry(container_of(node, NodeEntry, nodeId));
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    NodeEntry *oldEntry = container_of(oldNode, NodeEntry, nodeId);
    if(!oldEntry->deleted) {
        /* The nold version is not from the binfile */
        if(oldEntry != entry->orig) {
            /* The node was already updated since the copy was made */
            deleteEntry(entry);
            zipNsReleaseNode(nsCtx, oldNode);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        ZIP_REMOVE(NodeTreeBin, &ns->root, oldEntry);
        entry->nodeIdHash = oldEntry->nodeIdHash;
        oldEntry->deleted = true;
    } else {
        entry->nodeIdHash = UA_NodeId_hash(&node->nodeId);
    }

    /* Replace */
    ZIP_INSERT(NodeTreeBin, &ns->root, entry, ZIP_RANK(entry, zipfields));

    zipNsReleaseNode(nsCtx, oldNode);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
zipNsRemoveNode(void *nsCtx, const UA_NodeId *nodeId) {
    ZipContext *ns = (ZipContext*)nsCtx;
    NodeEntry dummy;
    dummy.nodeIdHash = UA_NodeId_hash(nodeId);
    dummy.nodeId = *nodeId;
    NodeEntry *entry = ZIP_FIND(NodeTreeBin, &ns->root, &dummy);
    if(!entry)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    ZIP_REMOVE(NodeTreeBin, &ns->root, entry);
    entry->deleted = true;
    cleanupEntry(entry);
    return UA_STATUSCODE_GOOD;
}

struct VisitorData {
    UA_NodestoreVisitor visitor;
    void *visitorContext;
};

static void
nodeVisitor(NodeEntry *entry, void *data) {
    struct VisitorData *d = (struct VisitorData*)data;
    d->visitor(d->visitorContext, (UA_Node*)&entry->nodeId);
}

static void
zipNsIterate(void *nsCtx, UA_NodestoreVisitor visitor,
             void *visitorCtx) {
    struct VisitorData d;
    d.visitor = visitor;
    d.visitorContext = visitorCtx;
    ZipContext *ns = (ZipContext*)nsCtx;
    ZIP_ITER(NodeTreeBin, &ns->root, nodeVisitor, &d);
}

static void
deleteNodeVisitor(NodeEntry *entry, void *data) {
    deleteEntry(entry);
}

/***********************/
/* Nodestore Lifecycle */
/***********************/

static void
zipNsClear(void *nsCtx) {
    if (!nsCtx)
        return;
    ZipContext *ns = (ZipContext*)nsCtx;
    ZIP_ITER(NodeTreeBin, &ns->root, deleteNodeVisitor, NULL);

    /* Clear nodeId contents */
    for (UA_UInt32 i = 0; i < ns->ltSize; i++) {
        UA_NodeId_clear(&ns->lte[i].ltRead.nodeId);
    }
    UA_free(&ns->lte[0]);
    UA_free(ns);
}

UA_StatusCode
UA_Nodestore_BinaryEncoded(UA_Nodestore *ns, const char *const lookupTablePath,
                           const char *const enocdedBinPath) {
    /* Allocate and initialize the context */
    ZipContext *ctx = (ZipContext*)UA_malloc(sizeof(ZipContext));
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ZIP_INIT(&ctx->root);
    ZIP_INIT(&ctx->ltRoot);

    /* Populate the nodestore */
    ns->context = (void*)ctx;
    ns->clear = zipNsClear;
    ns->newNode = zipNsNewNode;
    ns->deleteNode = zipNsDeleteNode;
    ns->getNode = zipNsGetNode;
    ns->releaseNode = zipNsReleaseNode;
    ns->getNodeCopy = zipNsGetNodeCopy;
    ns->insertNode = zipNsInsertNode;
    ns->replaceNode = zipNsReplaceNode;
    ns->removeNode = zipNsRemoveNode;
    ns->iterate = zipNsIterate;

    /* Initialize binary encoded nodes and lookuptable */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Read_Encoded_Binary(&ctx->encodeBin, enocdedBinPath);
    retval |= UA_Read_LookUpTable(ctx, lookupTablePath);

    return retval;
}
#endif

#ifdef UA_ENABLE_ENCODE_AND_DUMP

static size_t
commonVariableAttributeCalcSizeBinary(const UA_VariableNode *node) {
    size_t size = 0;
    size += UA_NodeId_calcSizeBinary(&node->dataType);
    size += UA_Int32_calcSizeBinary(&node->valueRank);
    size += UA_UInt64_calcSizeBinary(&node->arrayDimensionsSize);
    if(node->arrayDimensionsSize) {
        size += UA_UInt32_calcSizeBinary(node->arrayDimensions);
    }
    size += UA_UInt32_calcSizeBinary((const UA_UInt32*)&node->valueSource);
    UA_DataValue v1 = node->value.data.value;
    size += UA_DataValue_calcSizeBinary(&v1);
    return size;
}

static size_t
objectNodeCalcSizeBinary(const UA_ObjectNode* node) {
    return UA_Byte_calcSizeBinary(&node->eventNotifier);
}

static size_t
variableNodeCalcSizeBinary(const UA_VariableNode* node) {
    size_t size = 0;
    size += UA_Byte_calcSizeBinary(&node->accessLevel);
    size += UA_Double_calcSizeBinary(&node->minimumSamplingInterval);
    size += UA_Boolean_calcSizeBinary(&node->historizing);
    size += commonVariableAttributeCalcSizeBinary(node);
    return size;
}

static size_t
methodNodeCalcSizeBinary(const UA_MethodNode* node) {
    return UA_Boolean_calcSizeBinary(&node->executable);
}

static size_t
objectTypeNodeCalcSizeBinary(const UA_ObjectTypeNode* node) {
    return UA_Boolean_calcSizeBinary(&node->isAbstract);
}

static size_t
variableTypeNodeCalcSizeBinary(const UA_VariableTypeNode* node){
    size_t size = 0;
    size += UA_Boolean_calcSizeBinary(&node->isAbstract);
    size += commonVariableAttributeCalcSizeBinary((const UA_VariableNode*)node);
    return size;
}

static size_t
referenceTypeNodeCalcSizeBinary(const UA_ReferenceTypeNode* node) {
    size_t size = 0;
    size += UA_Boolean_calcSizeBinary(&node->isAbstract);
    size += UA_Boolean_calcSizeBinary(&node->symmetric);
    size += UA_LocalizedText_calcSizeBinary(&node->inverseName);
    return size;
}

static size_t
dataTypeNodeCalcSizeBinary(const UA_DataTypeNode* node) {
    return UA_Boolean_calcSizeBinary(&node->isAbstract);
}

static size_t
viewNodeCalcSizeBinary(const UA_ViewNode* node) {
    size_t size = 0;
    size += UA_Byte_calcSizeBinary(&node->eventNotifier);
    size += UA_Boolean_calcSizeBinary(&node->containsNoLoops);
    return size;
}

static size_t
getNodeSize(const UA_Node * node) {
    size_t nodeSize = 0;

    nodeSize += UA_NodeId_calcSizeBinary(&node->nodeId);
    nodeSize += UA_NodeClass_calcSizeBinary(&node->nodeClass);
    nodeSize += UA_QualifiedName_calcSizeBinary(&node->browseName);
    nodeSize += UA_LocalizedText_calcSizeBinary(&node->displayName);
    nodeSize += UA_LocalizedText_calcSizeBinary(&node->description);
    nodeSize += UA_UInt32_calcSizeBinary(&node->writeMask);
    nodeSize += UA_UInt64_calcSizeBinary(&node->referencesSize);

    for(size_t i = 0; i < node->referencesSize; i++) {
        UA_NodeReferenceKind *refKind = &node->references[i];
        nodeSize += UA_NodeId_calcSizeBinary(&refKind->referenceTypeId);
        nodeSize += UA_Boolean_calcSizeBinary(&refKind->isInverse);
        nodeSize += UA_UInt64_calcSizeBinary(&refKind->refTargetsSize);
        for(size_t j = 0; j < node->references[i].refTargetsSize; j++) {
            UA_ReferenceTarget *refTarget = &refKind->refTargets[j];
            nodeSize += UA_UInt32_calcSizeBinary(&refTarget->targetHash);
            nodeSize += UA_ExpandedNodeId_calcSizeBinary(&refTarget->target);
        }
    }

    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        nodeSize += objectNodeCalcSizeBinary((const UA_ObjectNode*)node);
        break;
    case UA_NODECLASS_VARIABLE:
        nodeSize += variableNodeCalcSizeBinary((const UA_VariableNode*)node);
        break;
    case UA_NODECLASS_METHOD:
        nodeSize += methodNodeCalcSizeBinary((const UA_MethodNode*)node);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        nodeSize += objectTypeNodeCalcSizeBinary((const UA_ObjectTypeNode*)node);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        nodeSize += variableTypeNodeCalcSizeBinary((const UA_VariableTypeNode*)node);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        nodeSize += referenceTypeNodeCalcSizeBinary((const UA_ReferenceTypeNode*)node);
        break;
    case UA_NODECLASS_DATATYPE:
        nodeSize += dataTypeNodeCalcSizeBinary((const UA_DataTypeNode*)node);
        break;
    case UA_NODECLASS_VIEW:
        nodeSize += viewNodeCalcSizeBinary((const UA_ViewNode*)node);
        break;
    default:
        break;
    }

    return nodeSize;
}

void
encodeNodeCallback(void *visitorCtx, const UA_Node *node) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* To calculate the binary size of the encoded node */
    size_t nodeSize = 0;

    /* To measure the total binary size of the encoded node and
     * resolve the starting position of next nodes*/
    static size_t totalSize = 0;

    size_t ltNodeSize;
    size_t ltNodePosition;
    FILE *fpEncoded;
    FILE *fpLookuptable;

    fpEncoded = fopen("encodedNode.bin", "a");
    if(!fpEncoded) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The opening of file encodedNode.bin failed");
    }

    fpLookuptable = fopen("lookupTable.bin", "a");
    if(!fpLookuptable) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The opening of file lookupTable.bin failed");
    }

    nodeSize =  getNodeSize(node);

    ltNodePosition = totalSize;
    totalSize += nodeSize;
    ltNodeSize = nodeSize;

    UA_STACKARRAY(UA_Byte, new_stackValueEncoding, nodeSize);
    UA_ByteString new_valueEncoding;
    new_valueEncoding.data = new_stackValueEncoding;
    new_valueEncoding.length = nodeSize;

    /* Encode the node */
    retval = UA_Node_encode(node, &new_valueEncoding);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The encoding of nodes failed with error : %s",
                                      UA_StatusCode_name(retval));
    }

    for (size_t i = 0; i < nodeSize; i++) {
        fprintf(fpEncoded, "%c", new_valueEncoding.data[i]);
    }

    /*For Numeric nodeId */
    if(node->nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        fprintf(fpLookuptable, "ns=%d;i=%u %lu %lu\n", node->nodeId.namespaceIndex,
                                node->nodeId.identifier.numeric, ltNodePosition, ltNodeSize);
    }

    /*For String nodeId */
    if(node->nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        fprintf(fpLookuptable, "ns=%d;s=%.*s %lu %lu\n", node->nodeId.namespaceIndex,
                                (int)node->nodeId.identifier.string.length,
                                node->nodeId.identifier.string.data, ltNodePosition, ltNodeSize);
    }

    /*For Guid nodeId */
    if(node->nodeId.identifierType == UA_NODEIDTYPE_GUID) {
        fprintf(fpLookuptable, "ns=%d;g=" UA_PRINTF_GUID_FORMAT " %lu %lu\n", node->nodeId.namespaceIndex,
                UA_PRINTF_GUID_DATA(node->nodeId.identifier.guid), ltNodePosition, ltNodeSize);
    }

    /*For ByteString nodeId */
    if(node->nodeId.identifierType == UA_NODEIDTYPE_BYTESTRING) {
        fprintf(fpLookuptable, "ns=%d;b=%.*s %lu %lu\n", node->nodeId.namespaceIndex,
                                (int)node->nodeId.identifier.byteString.length,
                                node->nodeId.identifier.byteString.data, ltNodePosition, ltNodeSize);
    }

    fclose(fpLookuptable);
    fclose(fpEncoded);
}
#endif
