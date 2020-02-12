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

#include <sys/mman.h>
#include <stdio.h>

#if defined UA_ENABLE_ENCODE_AND_DUMP || defined UA_ENABLE_USE_ENCODED_NODES

#ifndef UA_ENABLE_IMMUTABLE_NODES
#error The ROM-based Nodestore requires nodes to be replaced on write
#endif

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

static UA_StatusCode
getEncodeNodes (const UA_Node *node, UA_ByteString *new_valueEncoding) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_Byte *bufPos = new_valueEncoding->data;
    const UA_Byte *bufEnd = &new_valueEncoding->data[new_valueEncoding->length];
    retval |= UA_NodeId_encodeBinary(&node->nodeId, &bufPos, bufEnd);
    retval |= UA_NodeClass_encodeBinary(&node->nodeClass, &bufPos, bufEnd);
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
#define MINIMALNODECOUNT        45 // Total number of nodes in MINIMAL configuration
const UA_UInt32 minimalNodeIds[MINIMALNODECOUNT] =
    {UA_NS0ID_REFERENCES, UA_NS0ID_HASSUBTYPE, UA_NS0ID_AGGREGATES, UA_NS0ID_HIERARCHICALREFERENCES,
     UA_NS0ID_NONHIERARCHICALREFERENCES, UA_NS0ID_HASCHILD, UA_NS0ID_ORGANIZES, UA_NS0ID_HASEVENTSOURCE,
     UA_NS0ID_HASMODELLINGRULE, UA_NS0ID_HASENCODING, UA_NS0ID_HASDESCRIPTION,UA_NS0ID_HASTYPEDEFINITION,
     UA_NS0ID_GENERATESEVENT, UA_NS0ID_HASPROPERTY, UA_NS0ID_HASCOMPONENT, UA_NS0ID_HASNOTIFIER,
     UA_NS0ID_HASORDEREDCOMPONENT, UA_NS0ID_BASEDATATYPE, UA_NS0ID_BASEVARIABLETYPE, UA_NS0ID_BASEDATAVARIABLETYPE,
     UA_NS0ID_PROPERTYTYPE, UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_FOLDERTYPE, UA_NS0ID_ROOTFOLDER,
     UA_NS0ID_OBJECTSFOLDER, UA_NS0ID_TYPESFOLDER, UA_NS0ID_REFERENCETYPESFOLDER, UA_NS0ID_DATATYPESFOLDER,
     UA_NS0ID_VARIABLETYPESFOLDER, UA_NS0ID_OBJECTTYPESFOLDER, UA_NS0ID_EVENTTYPESFOLDER, UA_NS0ID_VIEWSFOLDER,
     UA_NS0ID_SERVER, UA_NS0ID_SERVER_SERVERARRAY, UA_NS0ID_SERVER_NAMESPACEARRAY, UA_NS0ID_SERVER_SERVERSTATUS,
     UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME, UA_NS0ID_SERVER_SERVERSTATUS_STATE, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO,
     UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME,
     UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION,
     UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE
    };

UA_Boolean isMinimalNodesAdded = UA_FALSE;
#define MAX_ROW_LENGTH         30  // Maximum length of a row in lookup table

lookUpTable *ltRead;
UA_UInt32 ltSizeRead;
UA_ByteString encodeBin;

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



static void
readRowLookuptable(char ltRow [], int length, lookUpTable *lt, int row) {
    /**
     * There are four contents per row
     * |IdentifierType|Identifier|nodePosition|nodeSize|
     * Each data is separated by space
     */
    size_t index = 0;
    int column = 1;
    char ltTemp[MAX_ROW_LENGTH] = {0};
    for (int j = 0 ; j < length; j++) {
        if(ltRow[j] != ' ') {
            ltTemp[index] = ltRow[j];
            index++;
        }
        if(ltRow[j] == ' ' || j == length - 1) {
            switch(column) {
            case 1:
                if(strtol(ltTemp, NULL, 10) == UA_NODEIDTYPE_NUMERIC) {
                    lt[row].nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
                }
                if(strtol(ltTemp, NULL, 10) == UA_NODEIDTYPE_STRING) {
                    lt[row].nodeId.identifierType = UA_NODEIDTYPE_STRING;
                }
                break;
            case 2:
                if(lt[row].nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                    lt[row].nodeId.identifier.numeric = (UA_UInt32)strtol(ltTemp, NULL, 10);
                }
                if(lt[row].nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                    lt[row].nodeId.identifier.string = UA_STRING_NULL;
                    lt[row].nodeId.identifier.string.length = index;
                    ltTemp[index] = '\0';
                    lt[row].nodeId.identifier.string = UA_String_fromChars(&ltTemp[0]);
                }
                break;
            case 3:
                lt[row].nodePosition = (size_t) strtol(ltTemp, NULL, 10);
                break;
            case 4:
                lt[row].nodeSize = (size_t) strtol(ltTemp, NULL, 10);
                break;
            default:
                break;
            }
            /* Clear the array contents */
            for(int i = 0; i < MAX_ROW_LENGTH; i++) {
                ltTemp[i] = 0;
            }
            column++;
            index = 0;
        }
    }
}

static lookUpTable*
UA_Lookuptable_Initialize(UA_UInt32 *ltSize, const char *const path) {
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
    *ltSize = nodeCount;
    lookUpTable *lt = (lookUpTable *)UA_calloc(*ltSize, sizeof(lookUpTable));
    return lt;
}

static void
UA_Read_LookUpTable(lookUpTable *lt, UA_UInt32 ltSize, const char* const path){
    int ch;
    int length = 0;
    int row = 0;
    char ltRow[MAX_ROW_LENGTH] = {0};
    FILE *fpLookuptable  = fopen(path, "r");
    if(!fpLookuptable) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The opening of file lookupTable.bin failed");
    }
    while((ch = fgetc(fpLookuptable)) != EOF) {
        switch(ch) {
        case '\n':
            readRowLookuptable(ltRow, length, lt, row); //separate lookuptable content from each row and populate
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

void
encodeEditedNode(const UA_Node *node, UA_ByteString *encodedBin,
                          lookUpTable *lt, UA_UInt32 ltSize) {

    /* Find the index location of node that is edited */
    size_t index;
    if(node->nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        for (size_t i = 0; i < ltSize; i++) {
            if(lt[i].nodeId.identifier.numeric == node->nodeId.identifier.numeric) {
                index = i;
                break;
            }
        }
    }

    if(node->nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        for (size_t j = 0; j < ltSize; j++) {
            if(lt[j].nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                if(UA_String_equal(&lt[j].nodeId.identifier.string, &node->nodeId.identifier.string)) {
                    index = j;
                    break;
                }
            }
        }
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    size_t nodeSize =  getNodeSize(node);
    UA_UInt32 lastNodeIndex = ltSize - 1;

    UA_STACKARRAY(UA_Byte, new_stackValueEncoding, nodeSize);
    UA_ByteString new_valueEncoding;
    new_valueEncoding.data = new_stackValueEncoding;
    new_valueEncoding.length = nodeSize;

    /* Encode the value */
    retval |= getEncodeNodes(node, &new_valueEncoding);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The encoding of edited nodes failed with error : %s",
                             UA_StatusCode_name(retval));
    }

    /* When the node size is unchanged replace old encoded content with new one*/
    if(lt[index].nodeSize == nodeSize) {
        for (size_t k = lt[index].nodePosition, m = 0; k < lt[index].nodePosition + nodeSize; k++, m++) {
            encodedBin->data[k] = new_valueEncoding.data[m];
        }
    }

    /* When the node size is modified replace old encoded content with new one*/
    else{
        /* Update the encoded binary */
        size_t nextNodePos = lt[index].nodePosition + lt[index].nodeSize;
        for(size_t i = lt[index].nodePosition;
                i < (lt[lastNodeIndex].nodePosition + lt[lastNodeIndex].nodeSize);
                i++, nextNodePos++) {
            encodedBin->data[i] = encodedBin->data[nextNodePos];
        }

        /* update the lookuptable */
        for(size_t j = index; j < lastNodeIndex; j++) {
            /* Clear the destination nodeId */
            UA_NodeId_clear(&lt[j].nodeId);

            UA_NodeId_copy(&lt[j+1].nodeId, &lt[j].nodeId);

            /* Clear the source nodeId */
            UA_NodeId_clear(&lt[j+1].nodeId);

            /* Update the node size and node position */
            lt[j].nodeSize = lt[j+1].nodeSize;
            lt[j].nodePosition = lt[j-1].nodePosition + lt[j-1].nodeSize;
        }

        /* Re-insert the updated node at the end (node with modified size) */

        /* Clear the old content in the last location */
        UA_NodeId_clear(&lt[lastNodeIndex].nodeId);
        lt[lastNodeIndex].nodeSize = nodeSize;

        /* Calculate the starting position from the previous node */
        lt[lastNodeIndex].nodePosition = lt[lastNodeIndex - 1].nodePosition + lt[lastNodeIndex - 1].nodeSize;
        lt[lastNodeIndex].nodeId.identifierType = node->nodeId.identifierType;
        if(node->nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
            lt[lastNodeIndex].nodeId.identifier.numeric = node->nodeId.identifier.numeric;
        }
        if(node->nodeId.identifierType == UA_NODEIDTYPE_STRING) {
            UA_String_copy(&node->nodeId.identifier.string, &lt[lastNodeIndex].nodeId.identifier.string);
        }

        /* Update the encoded edited node */
        for(size_t i = lt[lastNodeIndex].nodePosition, j = 0;
                i < (lt[lastNodeIndex].nodePosition + nodeSize);
                i++, j++) {
            encodedBin->data[i] = new_valueEncoding.data[j];
        }
    }
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

static void checkMinimalNodeIds(UA_NodeId nodeId) {
    static UA_UInt16 insertedNodeCount = 0;
    for(int i = 0; i < MINIMALNODECOUNT; i++) {
        if(nodeId.identifier.numeric == minimalNodeIds[i]) {
            insertedNodeCount++;
        }
    }
    if(insertedNodeCount == MINIMALNODECOUNT) {
        isMinimalNodesAdded = UA_TRUE;
    }
}

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

ZIP_HEAD(NodeTreeBin, NodeEntry);
typedef struct NodeTreeBin NodeTreeBin;

typedef struct {
    NodeTreeBin root;
} ZipContext;

ZIP_PROTTYPE(NodeTreeBin, NodeEntry, NodeEntry)
ZIP_IMPL(NodeTreeBin, NodeEntry, zipfields, NodeEntry, zipfields, cmpNodeId)

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
    deleteEntry(container_of(node, NodeEntry, nodeId));
}

static void
zipNsReleaseNode(void *nsCtx, const UA_Node *node) {
    if(!node)
        return;
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    UA_assert(entry->refCount > 0);
    --entry->refCount;
    if(entry->deleted) {
        /* Encode the edited node and replace the encoded content */
        if(node->nodeClass == UA_NODECLASS_VARIABLE) {
            encodeEditedNode(node, &encodeBin, ltRead, ltSizeRead);
        }
    }
    cleanupEntry(entry);
}

UA_Node*
decodeNode(void *ctx, UA_ByteString encodedBin, size_t offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId nodeID;
    retval |= UA_NodeId_decodeBinary(&encodedBin, &offset, &nodeID);
    UA_NodeClass nodeClass;
    retval |= UA_NodeClass_decodeBinary(&encodedBin, &offset, &nodeClass);
    UA_QualifiedName browseName;
    retval |= UA_QualifiedName_decodeBinary(&encodedBin, &offset, &browseName);
    UA_LocalizedText displayName;
    retval |= UA_LocalizedText_decodeBinary(&encodedBin, &offset, &displayName);
    UA_LocalizedText description;
    retval |= UA_LocalizedText_decodeBinary(&encodedBin, &offset, &description);
    UA_UInt32 writeMask;
    retval |= UA_UInt32_decodeBinary(&encodedBin, &offset, &writeMask);
    size_t referencesSize;
    retval |= UA_UInt64_decodeBinary(&encodedBin, &offset, (UA_UInt64 *)&referencesSize);

    UA_Node* node = zipNsNewNode(ctx, nodeClass);
    node->context = ctx;
    memcpy(&node->nodeId, &nodeID, sizeof(UA_NodeId));
    node->nodeClass = nodeClass;
    memcpy(&node->browseName, &browseName, sizeof(UA_QualifiedName));
    memcpy(&node->displayName, &displayName, sizeof(UA_LocalizedText));
    memcpy(&node->description, &description, sizeof(UA_LocalizedText));
    node->writeMask = writeMask;
    node->referencesSize = referencesSize;

    node->references = (UA_NodeReferenceKind*) UA_calloc(referencesSize, sizeof(UA_NodeReferenceKind));
    for (size_t i = 0; i < referencesSize; i++) {
        UA_NodeId referenceTypeId;
        retval |= UA_NodeId_decodeBinary(&encodedBin, &offset, &referenceTypeId);
        UA_Boolean isInverse;
        retval |= UA_Boolean_decodeBinary(&encodedBin, &offset, &isInverse);
        memcpy(&node->references[i].referenceTypeId, &referenceTypeId, sizeof(UA_NodeId));
        node->references[i].isInverse = isInverse;

           size_t refTargetsSize;
           retval |= UA_UInt64_decodeBinary(&encodedBin, &offset, (UA_UInt64 *)&refTargetsSize);
           node->references[i].refTargetsSize = refTargetsSize;
           node->references[i].refTargets = (UA_ReferenceTarget*)UA_calloc(node->references[i].refTargetsSize,
                                                 sizeof(UA_ReferenceTarget));
           for (size_t j = 0; j < refTargetsSize; j++) {
               UA_UInt32 targetHash;
            retval |= UA_UInt32_decodeBinary(&encodedBin, &offset, &targetHash);
            UA_ExpandedNodeId target;
            retval |= UA_ExpandedNodeId_decodeBinary(&encodedBin, &offset, &target);
            node->references[i].refTargets[j].targetHash = targetHash;
            memcpy(&node->references[i].refTargets[j].target, &target, sizeof(UA_ExpandedNodeId));
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
    if(isMinimalNodesAdded) {
        if(nodeId->identifierType == UA_NODEIDTYPE_NUMERIC) {
            for (UA_UInt32 i = 0; i < ltSizeRead; i++) {
                if(ltRead[i].nodeId.identifier.numeric == nodeId->identifier.numeric) {
                    const UA_Node* node = decodeNode(nsCtx, encodeBin, ltRead[i].nodePosition);
                    /* Datasource variable to be redirected to the uncompressed nodes i.e., Datetime
                     * TODO: Redirect UA_NODECLASS_METHOD to uncompressed nodes */
                    if(node->nodeClass == UA_NODECLASS_VARIABLE) {
                        const UA_VariableTypeNode *varNode = (const UA_VariableTypeNode*) node;
                        if(varNode->valueSource == UA_VALUESOURCE_DATASOURCE) {
                            zipNsReleaseNode(nsCtx, node); // Delete the above allocated memory!
                            goto getnode;
                        }
                    }
                    return node;
                }
            }
        }

        if(nodeId->identifierType == UA_NODEIDTYPE_STRING) {
            for (UA_UInt32 i = 0; i < ltSizeRead; i++) {
                if(ltRead[i].nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                    if(UA_String_equal(&ltRead[i].nodeId.identifier.string, &nodeId->identifier.string)) {
                        const UA_Node* node = decodeNode(nsCtx, encodeBin, ltRead[i].nodePosition);
                        /* Datasource variable to be redirected to the uncompressed nodes i.e., Datetime
                         * TODO: Redirect UA_NODECLASS_METHOD to uncompressed nodes */
                        if(node->nodeClass == UA_NODECLASS_VARIABLE) {
                            const UA_VariableTypeNode *varNode = (const UA_VariableTypeNode*) node;
                            if(varNode->valueSource == UA_VALUESOURCE_DATASOURCE) {
                                zipNsReleaseNode(nsCtx, node); // Delete the above allocated memory!
                                goto getnode;
                            }
                        }
                        return node;
                    }
                }
            }
        }
    }
    getnode:
    dummy.nodeIdHash = UA_NodeId_hash(nodeId);
    dummy.nodeId = *nodeId;
    NodeEntry *entry = ZIP_FIND(NodeTreeBin, &ns->root, &dummy);
    if(!entry)
        return NULL;
    ++entry->refCount;
    return (const UA_Node*)&entry->nodeId;
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

    /* Check if all the minimal nodes are inserted*/
    checkMinimalNodeIds(node->nodeId);

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
    /* Find the node */
    const UA_Node *oldNode = zipNsGetNode(nsCtx, &node->nodeId);
    if(!oldNode) {
        deleteEntry(container_of(node, NodeEntry, nodeId));
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Test if the copy is current */
    NodeEntry *entry = container_of(node, NodeEntry, nodeId);
    NodeEntry *oldEntry = container_of(oldNode, NodeEntry, nodeId);
    if(oldEntry != entry->orig) {
        /* The node was already updated since the copy was made */
        deleteEntry(entry);
        zipNsReleaseNode(nsCtx, oldNode);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Replace */
    ZipContext *ns = (ZipContext*)nsCtx;
    ZIP_REMOVE(NodeTreeBin, &ns->root, oldEntry);
    entry->nodeIdHash = oldEntry->nodeIdHash;
    ZIP_INSERT(NodeTreeBin, &ns->root, entry, ZIP_RANK(entry, zipfields));
    oldEntry->deleted = true;

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
    UA_free(ns);

    /* Clear encoded node contents */
    for (UA_UInt32 i = 0; i < ltSizeRead; i++) {
        UA_NodeId_clear(&ltRead[i].nodeId);
    }
    UA_free(ltRead);
    //UA_free(encodeBin.data); // mmapped
}

UA_StatusCode
UA_Nodestore_BinaryEncoded(UA_Nodestore *ns, const char *const lookupTablePath,
		                   const char *const enocdedBinPath) {
    /* Allocate and initialize the context */
    ZipContext *ctx = (ZipContext*)UA_malloc(sizeof(ZipContext));
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ZIP_INIT(&ctx->root);

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

    /* Initialize binary enocded nodes and lookuptable */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Read_Encoded_Binary(&encodeBin, enocdedBinPath);
    ltRead = UA_Lookuptable_Initialize(&ltSizeRead, lookupTablePath);
    UA_Read_LookUpTable(&ltRead[0], ltSizeRead, lookupTablePath);

    return retval;
}
#endif

#ifdef UA_ENABLE_ENCODE_AND_DUMP
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
    retval = getEncodeNodes(node, &new_valueEncoding);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The encoding of nodes failed with error : %s",
                                      UA_StatusCode_name(retval));
    }

    for (size_t i = 0; i < nodeSize; i++) {
        fprintf(fpEncoded, "%c", new_valueEncoding.data[i]);
    }

    if(node->nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        fprintf(fpLookuptable, "%d %u %lu %lu\n", node->nodeId.identifierType, node->nodeId.identifier.numeric,
                                          ltNodePosition, ltNodeSize);
    }

    if(node->nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        fprintf(fpLookuptable, "%d %s %lu %lu\n", node->nodeId.identifierType, node->nodeId.identifier.string.data,
                                          ltNodePosition, ltNodeSize);
    }

    fclose(fpLookuptable);
    fclose(fpEncoded);
}
#endif

