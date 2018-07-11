#!/usr/bin/env/python
# -*- coding: utf-8 -*-

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
### Version: rev 13
###
### This program was created for educational purposes and has been
### contributed to the open62541 project by the author. All licensing
### terms for this source is inherited by the terms and conditions
### specified for by the open62541 project (see the projects readme
### file for more information on the LGPL terms and restrictions).
###
### This program is not meant to be used in a production environment. The
### author is not liable for any complications arising due to the use of
### this program.
###

from nodes import *
from backend_open62541_datatypes import *
import re
import datetime
import logging

logger = logging.getLogger(__name__)

#################
# Generate Code #
#################

def generateNodeIdPrintable(node):
    CodePrintable = "NODE_"

    if isinstance(node.id, NodeId):
        CodePrintable = node.__class__.__name__ + "_" + str(node.id)
    else:
        CodePrintable = node.__class__.__name__ + "_unknown_nid"

    return re.sub('[^0-9a-z_]+', '_', CodePrintable.lower())

def generateNodeValueInstanceName(node, parent, recursionDepth, arrayIndex):
    return generateNodeIdPrintable(parent) + "_" + str(node.alias) + "_" + str(arrayIndex) + "_" + str(recursionDepth)

def generateReferenceCode(reference):
    if reference.isForward:
        return "retVal |= UA_Server_addReference(server, %s, %s, %s, true);" % \
               (generateNodeIdCode(reference.source),
                generateNodeIdCode(reference.referenceType),
                generateExpandedNodeIdCode(reference.target))
    else:
        return "retVal |= UA_Server_addReference(server, %s, %s, %s, false);" % \
               (generateNodeIdCode(reference.source),
                generateNodeIdCode(reference.referenceType),
                generateExpandedNodeIdCode(reference.target))

def generateReferenceTypeNodeCode(node):
    code = []
    code.append("UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    if node.symmetric:
        code.append("attr.symmetric  = true;")
    if node.inverseName != "":
        code.append("attr.inverseName  = UA_LOCALIZEDTEXT(\"\", \"%s\");" % \
                    node.inverseName)
    return code

def generateObjectNodeCode(node):
    code = []
    code.append("UA_ObjectAttributes attr = UA_ObjectAttributes_default;")
    if node.eventNotifier:
        code.append("attr.eventNotifier = true;")
    return code

def generateVariableNodeCode(node, nodeset, max_string_length, encode_binary_size):
    code = []
    codeCleanup = []
    code.append("UA_VariableAttributes attr = UA_VariableAttributes_default;")
    if node.historizing:
        code.append("attr.historizing = true;")
    code.append("attr.minimumSamplingInterval = %f;" % node.minimumSamplingInterval)
    code.append("attr.userAccessLevel = %d;" % node.userAccessLevel)
    code.append("attr.accessLevel = %d;" % node.accessLevel)
    # in order to be compatible with mostly OPC UA client
    # force valueRank = -1 for scalar VariableNode
    if node.valueRank == -2:
        node.valueRank = -1
    code.append("attr.valueRank = %d;" % node.valueRank)
    if node.valueRank > 0:
        code.append("attr.arrayDimensionsSize = %d;" % node.valueRank)
        code.append("attr.arrayDimensions = (UA_UInt32 *)UA_Array_new({}, &UA_TYPES[UA_TYPES_UINT32]);".format(node.valueRank))
        codeCleanup.append("UA_Array_delete(attr.arrayDimensions, {}, &UA_TYPES[UA_TYPES_UINT32]);".format(node.valueRank))
        if len(node.arrayDimensions) == node.valueRank:
            for idx, v in enumerate(node.arrayDimensions):
                code.append("attr.arrayDimensions[{}] = {};".format(idx, int(unicode(v))))
        else:
            for dim in range(0, node.valueRank):
                code.append("attr.arrayDimensions[{}] = 0;".format(dim))

    if node.dataType is not None:
        if isinstance(node.dataType, NodeId) and node.dataType.ns == 0 and node.dataType.i == 0:
            #BaseDataType
            dataTypeNode = nodeset.nodes[NodeId("i=24")]
            dataTypeNodeOpaque = nodeset.nodes[NodeId("i=24")]
        else:
            dataTypeNodeOpaque = nodeset.getDataTypeNode(node.dataType)
            dataTypeNode = nodeset.getBaseDataType(nodeset.getDataTypeNode(node.dataType))

        if dataTypeNode is not None:
            code.append("attr.dataType = %s;" % generateNodeIdCode(dataTypeNodeOpaque.id))

            if dataTypeNode.isEncodable():
                if node.value is not None:
                    [code1, codeCleanup1] = generateValueCode(node.value, nodeset.nodes[node.id], nodeset, max_string_length=max_string_length, encode_binary_size=encode_binary_size)
                    code += code1
                    codeCleanup += codeCleanup1
                    if node.valueRank > 0 and len(node.arrayDimensions) == node.valueRank:
                        code.append("attr.value.arrayDimensionsSize = attr.arrayDimensionsSize;")
                        code.append("attr.value.arrayDimensions = attr.arrayDimensions;")
                else:
                    code += generateValueCodeDummy(dataTypeNode, nodeset.nodes[node.id], nodeset)
    return [code, codeCleanup]

def generateVariableTypeNodeCode(node, nodeset, max_string_length, encode_binary_size):
    code = []
    codeCleanup = []
    code.append("UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;")
    if node.historizing:
        code.append("attr.historizing = true;")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    code.append("attr.valueRank = (UA_Int32)%s;" % str(node.valueRank))
    if node.dataType is not None:
        if isinstance(node.dataType, NodeId) and node.dataType.ns == 0 and node.dataType.i == 0:
            #BaseDataType
            dataTypeNode = nodeset.nodes[NodeId("i=24")]
        else:
            dataTypeNode = nodeset.getBaseDataType(nodeset.getDataTypeNode(node.dataType))
        if dataTypeNode is not None:
            code.append("attr.dataType = %s;" % generateNodeIdCode(dataTypeNode.id))
            if dataTypeNode.isEncodable():
                if node.value is not None:
                    [code1, codeCleanup1] = generateValueCode(node.value, nodeset.nodes[node.id], nodeset, max_string_length, encode_binary_size)
                    code += code1
                    codeCleanup += codeCleanup1
                else:
                    code += generateValueCodeDummy(dataTypeNode, nodeset.nodes[node.id], nodeset)
    return [code, codeCleanup]

def generateExtensionObjectSubtypeCode(node, parent, nodeset, recursionDepth=0, arrayIndex=0, max_string_length=0, encode_binary_size=32000):
    code = [""]
    codeCleanup = [""]

    logger.debug("Building extensionObject for " + str(parent.id))
    logger.debug("Value    " + str(node.value))
    logger.debug("Encoding " + str(node.encodingRule))

    instanceName = generateNodeValueInstanceName(node, parent, recursionDepth, arrayIndex)
    # If there are any ExtensionObjects instide this ExtensionObject, we need to
    # generate one-time-structs for them too before we can proceed;
    for subv in node.value:
        if isinstance(subv, list):
            logger.error("ExtensionObject contains an ExtensionObject, which is currently not encodable!")

    code.append("struct {")
    for field in node.encodingRule:
        ptrSym = ""
        # If this is an Array, this is pointer to its contents with a AliasOfFieldSize entry
        if field[2] != 0:
            code.append("  UA_Int32 " + str(field[0]) + "Size;")
            ptrSym = "*"
        if len(field[1]) == 1:
            code.append("  UA_" + str(field[1][0]) + " " + ptrSym + str(field[0]) + ";")
        else:
            code.append("  UA_ExtensionObject " + " " + ptrSym + str(field[0]) + ";")
    code.append("} " + instanceName + "_struct;")

    # Assign data to the struct contents
    # Track the encoding rule definition to detect arrays and/or ExtensionObjects
    encFieldIdx = 0
    for subv in node.value:
        encField = node.encodingRule[encFieldIdx]
        encFieldIdx = encFieldIdx + 1
        logger.debug(
            "Encoding of field " + subv.alias + " is " + str(subv.encodingRule) + "defined by " + str(encField))
        # Check if this is an array
        if encField[2] == 0:
            code.append(instanceName + "_struct." + subv.alias + " = " +
                        generateNodeValueCode(subv, instanceName, asIndirect=False, max_string_length=max_string_length) + ";")
        else:
            if isinstance(subv, list):
                # this is an array
                code.append(instanceName + "_struct." + subv.alias + "Size = " + str(len(subv)) + ";")
                code.append(
                     "{0}_struct.{1} = (UA_{2}*) UA_malloc(sizeof(UA_{2})*{3});".format(
                         instanceName, subv.alias, subv.__class__.__name__, str(len(subv))))
                codeCleanup.append("UA_free({0}_struct.{1});".format(instanceName, subv.alias))
                logger.debug("Encoding included array of " + str(len(subv)) + " values.")
                for subvidx in range(0, len(subv)):
                    subvv = subv[subvidx]
                    logger.debug("  " + str(subvidx) + " " + str(subvv))
                    code.append(instanceName + "_struct." + subv.alias + "[" + str(
                        subvidx) + "] = " + generateNodeValueCode(subvv, instanceName, max_string_length=max_string_length) + ";")
                code.append("}")
            else:
                code.append(instanceName + "_struct." + subv.alias + "Size = 1;")
                code.append(
                    "{0}_struct.{1} = (UA_{2}*) UA_malloc(sizeof(UA_{2}));".format(
                        instanceName, subv.alias, subv.__class__.__name__))
                codeCleanup.append("UA_free({0}_struct.{1});".format(instanceName, subv.alias))

                code.append(instanceName + "_struct." + subv.alias + "[0]  = " +
                            generateNodeValueCode(subv, instanceName, asIndirect=True, max_string_length=max_string_length) + ";")

    # Allocate some memory
    code.append("UA_ExtensionObject *" + instanceName + " =  UA_ExtensionObject_new();")
    codeCleanup.append("UA_ExtensionObject_delete(" + instanceName + ");")
    code.append(instanceName + "->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;")
    #if parent.dataType.ns == 0:

    binaryEncodingId = nodeset.getBinaryEncodingIdForNode(parent.dataType)
    code.append(
        instanceName + "->content.encoded.typeId = UA_NODEID_NUMERIC(" + str(binaryEncodingId.ns) + ", " +
        str(binaryEncodingId.i) + ");")
    code.append(
        "UA_ByteString_allocBuffer(&" + instanceName + "->content.encoded.body, " + str(encode_binary_size) + ");")

    # Encode each value as a bytestring separately.
    code.append("UA_Byte *pos" + instanceName + " = " + instanceName + "->content.encoded.body.data;")
    code.append("const UA_Byte *end" + instanceName + " = &" + instanceName + "->content.encoded.body.data[" + str(encode_binary_size) + "];")
    encFieldIdx = 0
    code.append("{")
    for subv in node.value:
        encField = node.encodingRule[encFieldIdx]
        encFieldIdx = encFieldIdx + 1
        if encField[2] == 0:
            code.append(
                "retVal |= UA_encodeBinary(&" + instanceName + "_struct." + subv.alias + ", " +
                getTypesArrayForValue(nodeset, subv) + ", &pos" + instanceName + ", &end" + instanceName + ", NULL, NULL);")
        else:
            if isinstance(subv, list):
                for subvidx in range(0, len(subv)):
                    code.append("retVal |= UA_encodeBinary(&" + instanceName + "_struct." + subv.alias + "[" +
                                str(subvidx) + "], " + getTypesArrayForValue(nodeset, subv) + ", &pos" +
                                instanceName + ", &end" + instanceName + ", NULL, NULL);")
            else:
                code.append(
                    "retVal |= UA_encodeBinary(&" + instanceName + "_struct." + subv.alias + "[0], " +
                    getTypesArrayForValue(nodeset, subv) + ", &pos" + instanceName + ", &end" + instanceName + ", NULL, NULL);")

    code.append("}")
    # Reallocate the memory by swapping the 65k Bytestring for a new one
    code.append("size_t " + instanceName + "_encOffset = (uintptr_t)(" +
                "pos" + instanceName + "-" + instanceName + "->content.encoded.body.data);")
    code.append(instanceName + "->content.encoded.body.length = " + instanceName + "_encOffset;")
    code.append("UA_Byte *" + instanceName + "_newBody = (UA_Byte *) UA_malloc(" + instanceName + "_encOffset);")
    code.append("memcpy(" + instanceName + "_newBody, " + instanceName + "->content.encoded.body.data, " +
                instanceName + "_encOffset);")
    code.append("UA_Byte *" + instanceName + "_oldBody = " + instanceName + "->content.encoded.body.data;")
    code.append(instanceName + "->content.encoded.body.data = " + instanceName + "_newBody;")
    code.append("UA_free(" + instanceName + "_oldBody);")
    code.append("")
    return [code, codeCleanup]


def generateValueCodeDummy(dataTypeNode, parentNode, nodeset):
    code = []
    valueName = generateNodeIdPrintable(parentNode) + "_variant_DataContents"

    typeBrowseNode = dataTypeNode.browseName.name
    if typeBrowseNode == "NumericRange":
        # in the stack we define a separate structure for the numeric range, but
        # the value itself is just a string
        typeBrowseNode = "String"

    typeArr = dataTypeNode.typesArray + "[" + dataTypeNode.typesArray + "_" + typeBrowseNode.upper() + "]"
    typeStr = "UA_" + typeBrowseNode

    if parentNode.valueRank > 0:
        for i in range(0, parentNode.valueRank):
            code.append("UA_Variant_setArray(&attr.value, NULL, (UA_Int32) " + "0, &" + typeArr + ");")
    elif not dataTypeNode.isAbstract:
        code.append("UA_STACKARRAY(" + typeStr + ", " + valueName + ", 1);")
        code.append("UA_init(" + valueName + ", &" + typeArr + ");")
        code.append("UA_Variant_setScalar(&attr.value, " + valueName + ", &" + typeArr + ");")

    return code

def getTypesArrayForValue(nodeset, value):
    typeNode = nodeset.getNodeByBrowseName(value.__class__.__name__)
    if typeNode is None or value.isInternal:
        typesArray = "UA_TYPES"
    else:
        typesArray = typeNode.typesArray
    return "&" + typesArray + "[" + typesArray + "_" + \
                    value.__class__.__name__.upper() + "]"

def generateValueCode(node, parentNode, nodeset, bootstrapping=True, max_string_length=0, encode_binary_size=32000):
    code = []
    codeCleanup = []
    valueName = generateNodeIdPrintable(parentNode) + "_variant_DataContents"

    # node.value either contains a list of multiple identical BUILTINTYPES, or it
    # contains a single builtintype (which may be a container); choose if we need
    # to create an array or a single variable.
    # Note that some genious defined that there are arrays of size 1, which are
    # distinctly different then a single value, so we need to check that as well
    # Semantics:
    # -3: Scalar or 1-dim
    # -2: Scalar or x-dim | x>0
    # -1: Scalar
    #  0: x-dim | x>0
    #  n: n-dim | n>0
    if (len(node.value) == 0):
        return ["", ""]
    if not isinstance(node.value[0], Value):
        return ["", ""]

    if parentNode.valueRank != -1 and (parentNode.valueRank >= 0
                                       or (len(node.value) > 1
                                           and (parentNode.valueRank != -2 or parentNode.valueRank != -3))):
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if isinstance(node.value[0], Guid):
            logger.warn("Don't know how to print array of GUID in node " + str(parentNode.id))
        elif isinstance(node.value[0], DiagnosticInfo):
            logger.warn("Don't know how to print array of DiagnosticInfo in node " + str(parentNode.id))
        elif isinstance(node.value[0], StatusCode):
            logger.warn("Don't know how to print array of StatusCode in node " + str(parentNode.id))
        else:
            if isinstance(node.value[0], ExtensionObject):
                for idx, v in enumerate(node.value):
                    logger.debug("Building extObj array index " + str(idx))
                    [code1, codeCleanup1] = generateExtensionObjectSubtypeCode(v, parent=parentNode, nodeset=nodeset, arrayIndex=idx, max_string_length=max_string_length,
                                                                               encode_binary_size=encode_binary_size)
                    code = code + code1
                    codeCleanup = codeCleanup + codeCleanup1
            code.append("UA_" + node.value[0].__class__.__name__ + " " + valueName + "[" + str(len(node.value)) + "];")
            if isinstance(node.value[0], ExtensionObject):
                for idx, v in enumerate(node.value):
                    logger.debug("Printing extObj array index " + str(idx))
                    instanceName = generateNodeValueInstanceName(v, parentNode, 0, idx)
                    code.append(
                        valueName + "[" + str(idx) + "] = " +
                        generateNodeValueCode(v, instanceName, max_string_length=max_string_length) + ";")
                    # code.append("UA_free(&" +valueName + "[" + str(idx) + "]);")
            else:
                for idx, v in enumerate(node.value):
                    instanceName = generateNodeValueInstanceName(v, parentNode, 0, idx)
                    code.append(
                        valueName + "[" + str(idx) + "] = " + generateNodeValueCode(v, instanceName, max_string_length=max_string_length) + ";")
            code.append("UA_Variant_setArray(&attr.value, &" + valueName +
                        ", (UA_Int32) " + str(len(node.value)) + ", " +
                        getTypesArrayForValue(nodeset, node.value[0]) + ");")
    else:
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if isinstance(node.value[0], Guid):
            logger.warn("Don't know how to print scalar GUID in node " + str(parentNode.id))
        elif isinstance(node.value[0], DiagnosticInfo):
            logger.warn("Don't know how to print scalar DiagnosticInfo in node " + str(parentNode.id))
        elif isinstance(node.value[0], StatusCode):
            logger.warn("Don't know how to print scalar StatusCode in node " + str(parentNode.id))
        else:
            # The following strategy applies to all other types, in particular strings and numerics.
            if isinstance(node.value[0], ExtensionObject):
                [code1, codeCleanup1] = generateExtensionObjectSubtypeCode(node.value[0], parent=parentNode, nodeset=nodeset, max_string_length=max_string_length,
                                                                           encode_binary_size=encode_binary_size)
                code = code + code1
                codeCleanup = codeCleanup + codeCleanup1
            instanceName = generateNodeValueInstanceName(node.value[0], parentNode, 0, 0)
            if isinstance(node.value[0], ExtensionObject):
                code.append("UA_" + node.value[0].__class__.__name__ + " *" + valueName + " = " +
                            generateNodeValueCode(node.value[0], instanceName, max_string_length=max_string_length) + ";")
                code.append(
                    "UA_Variant_setScalar(&attr.value, " + valueName + ", " +
                    getTypesArrayForValue(nodeset, node.value[0]) + ");")

                # FIXME: There is no membership definition for extensionObjects generated in this function.
                # code.append("UA_" + node.value[0].__class__.__name__ + "_deleteMembers(" + valueName + ");")
            else:
                code.append("UA_" + node.value[0].__class__.__name__ + " *" + valueName + " =  UA_" + node.value[
                    0].__class__.__name__ + "_new();")
                code.append("*" + valueName + " = " + generateNodeValueCode(node.value[0], instanceName, asIndirect=True, max_string_length=max_string_length) + ";")
                code.append(
                        "UA_Variant_setScalar(&attr.value, " + valueName + ", " +
                        getTypesArrayForValue(nodeset, node.value[0]) + ");")
                codeCleanup.append("UA_{0}_delete({1});".format(
                    node.value[0].__class__.__name__, valueName))
    return [code, codeCleanup]

def generateMethodNodeCode(node):
    code = []
    code.append("UA_MethodAttributes attr = UA_MethodAttributes_default;")
    if node.executable:
        code.append("attr.executable = true;")
    if node.userExecutable:
        code.append("attr.userExecutable = true;")
    return code

def generateObjectTypeNodeCode(node):
    code = []
    code.append("UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    return code

def generateDataTypeNodeCode(node):
    code = []
    code.append("UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    return code

def generateViewNodeCode(node):
    code = []
    code.append("UA_ViewAttributes attr = UA_ViewAttributes_default;")
    if node.containsNoLoops:
        code.append("attr.containsNoLoops = true;")
    code.append("attr.eventNotifier = (UA_Byte)%s;" % str(node.eventNotifier))
    return code

def getNodeTypeDefinition(node):
    for ref in node.references:
        # 40 = HasTypeDefinition
        if ref.referenceType.i == 40:
            return ref.target
    return None

def generateSubtypeOfDefinitionCode(node):
    for ref in node.inverseReferences:
        # 45 = HasSubtype
        if ref.referenceType.i == 45:
            return generateNodeIdCode(ref.target)
    return "UA_NODEID_NULL"

def generateNodeCode_begin(node, nodeset, max_string_length, generate_ns0, parentref, encode_binary_size):
    code = []
    codeCleanup = []
    code.append("UA_StatusCode retVal = UA_STATUSCODE_GOOD;")

    # Attributes
    if isinstance(node, ReferenceTypeNode):
        code.extend(generateReferenceTypeNodeCode(node))
    elif isinstance(node, ObjectNode):
        code.extend(generateObjectNodeCode(node))
    elif isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        [code1, codeCleanup1] = generateVariableNodeCode(node, nodeset, max_string_length, encode_binary_size)
        code.extend(code1)
        codeCleanup.extend(codeCleanup1)
    elif isinstance(node, VariableTypeNode):
        [code1, codeCleanup1] = generateVariableTypeNodeCode(node, nodeset, max_string_length, encode_binary_size)
        code.extend(code1)
        codeCleanup.extend(codeCleanup1)
    elif isinstance(node, MethodNode):
        code.extend(generateMethodNodeCode(node))
    elif isinstance(node, ObjectTypeNode):
        code.extend(generateObjectTypeNodeCode(node))
    elif isinstance(node, DataTypeNode):
        code.extend(generateDataTypeNodeCode(node))
    elif isinstance(node, ViewNode):
        code.extend(generateViewNodeCode(node))
    code.append("attr.displayName = " + generateLocalizedTextCode(node.displayName, alloc=False,
                                                                  max_string_length=max_string_length) + ";")
    code.append("#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS")
    code.append("attr.description = " + generateLocalizedTextCode(node.description, alloc=False,
                                                                  max_string_length=max_string_length) + ";")
    code.append("#endif")
    code.append("attr.writeMask = %d;" % node.writeMask)
    code.append("attr.userWriteMask = %d;" % node.userWriteMask)

    # AddNodes call
    code.append("retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_{},".
                format(node.__class__.__name__.upper().replace("NODE" ,"")))
    code.append(generateNodeIdCode(node.id) + ",")
    code.append(generateNodeIdCode(parentref.target) + ",")
    code.append(generateNodeIdCode(parentref.referenceType) + ",")
    code.append(generateQualifiedNameCode(node.browseName, max_string_length=max_string_length) + ",")
    if isinstance(node, VariableNode) or isinstance(node, ObjectNode):
        typeDefRef = node.popTypeDef()
        code.append(generateNodeIdCode(typeDefRef.target) + ",")
    else:
        code.append(" UA_NODEID_NULL,")
    code.append("(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_{}ATTRIBUTES],NULL, NULL);".
                format(node.__class__.__name__.upper().replace("NODE" ,"")))
    code.extend(codeCleanup)
    
    return "\n".join(code)

def generateNodeCode_finish(node):
    code = []

    if isinstance(node, MethodNode):
        code.append("UA_Server_addMethodNode_finish(server, ")
    else:
        code.append("UA_Server_addNode_finish(server, ")
    code.append(generateNodeIdCode(node.id))

    if isinstance(node, MethodNode):
        code.append(", NULL, 0, NULL, 0, NULL);")
    else:
        code.append(");")

    return "\n".join(code)

