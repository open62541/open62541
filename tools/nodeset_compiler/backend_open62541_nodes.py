#!/usr/bin/env python3
# -*- coding: utf-8 -*-

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
###    Copyright 2019 (c) Andrea Minosu
###    Copyright 2018 (c) Jannis Volker
###    Copyright 2018 (c) Ralph Lange

from datatypes import  ExtensionObject, NodeId, StatusCode, DiagnosticInfo, Value
from nodes import ReferenceTypeNode, ObjectNode, VariableNode, VariableTypeNode, MethodNode, ObjectTypeNode, DataTypeNode, ViewNode
from backend_open62541_datatypes import makeCIdentifier, generateLocalizedTextCode, generateQualifiedNameCode, generateNodeIdCode, \
    generateExpandedNodeIdCode, generateNodeValueCode
import re
import logging

import sys

if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s

logger = logging.getLogger(__name__)

#################
# Generate Code #
#################

def generateNodeIdPrintable(node):
    if isinstance(node.id, NodeId):
        CodePrintable = node.__class__.__name__ + "_" + str(node.id)
    else:
        CodePrintable = node.__class__.__name__ + "_unknown_nid"

    return re.sub('[^0-9a-z_]+', '_', CodePrintable.lower())

def generateNodeValueInstanceName(node, parent, arrayIndex):
    return generateNodeIdPrintable(parent) + "_" + str(node.alias) + "_" + str(arrayIndex)

def generateReferenceCode(reference):
    code = []
    forwardFlag = "true" if reference.isForward else "false"
    code.append("retVal |= UA_Server_addReference(server, %s, %s, %s, %s);" %
                (generateNodeIdCode(reference.source),
                 generateNodeIdCode(reference.referenceType),
                 generateExpandedNodeIdCode(reference.target),
                 forwardFlag))
    code.append("if (retVal != UA_STATUSCODE_GOOD) return retVal;")
    return "\n".join(code)

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
        code_part = "attr.eventNotifier = "
        is_first = True
        if node.eventNotifier & 1:
            code_part += "UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT"
            is_first = False
        if node.eventNotifier & 4:
            if not is_first:
                code_part += " || "
            code_part += "UA_EVENTNOTIFIER_HISTORY_READ"
            is_first = False
        if node.eventNotifier & 8:
            if not is_first:
                code_part += " || "
            code_part += "UA_EVENTNOTIFIER_HISTORY_WRITE"
            is_first = False
        code_part += ";"
        code.append(code_part)
    return code

def setNodeDatatypeRecursive(node, nodeset):

    if not isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        raise RuntimeError("Node {}: DataType can only be set for VariableNode and VariableTypeNode".format(str(node.id)))

    if node.dataType is not None:
        return

    # If BaseVariableType
    if node.id == NodeId("ns=0;i=62"):
        if node.dataType is None:
            # Set to default BaseDataType
            node.dataType = NodeId("ns=0;i=24")
        return

    if isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        typeDefNode = nodeset.getNodeTypeDefinition(node)
        if typeDefNode is None:
            # Use the parent type.
            raise RuntimeError("Cannot get node for HasTypeDefinition of VariableNode " + node.browseName.name + " " + str(node.id))

        setNodeDatatypeRecursive(typeDefNode, nodeset)

        node.dataType = typeDefNode.dataType
    else:
        # Use the parent type.
        if node.parent is None:
            raise RuntimeError("Parent node not defined for " + node.browseName.name + " " + str(node.id))

        setNodeDatatypeRecursive(node.parent, nodeset)
        node.dataType = node.parent.dataType

def setNodeValueRankRecursive(node, nodeset):

    if not isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        raise RuntimeError("Node {}: ValueRank can only be set for VariableNode and VariableTypeNode".format(str(node.id)))

    if node.valueRank is not None:
        return

    # If BaseVariableType
    if node.id == NodeId("ns=0;i=62"):
        if node.valueRank is None:
            # BaseVariableType always has -2
            node.valueRank = -2
        return

    if isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        typeDefNode = nodeset.getNodeTypeDefinition(node)
        if typeDefNode is None:
            # Use the parent type.
            raise RuntimeError("Cannot get node for HasTypeDefinition of VariableNode " + node.browseName.name + " " + str(node.id))
        if not isinstance(typeDefNode, VariableTypeNode):
            raise RuntimeError("Node {} ({}) has an invalid type definition. {} is not a VariableType node.".format(
                str(node.id), node.browseName.name, str(typeDefNode.id)))


        setNodeValueRankRecursive(typeDefNode, nodeset)

        if typeDefNode.valueRank is not None:
            node.valueRank = typeDefNode.valueRank
        else:
            raise RuntimeError("Node {}: the ValueRank of the parent node is None.".format(str(node.id)))
    else:
        if node.parent is None:
            raise RuntimeError("Node {}: does not have a parent. Probably the parent node was blacklisted?".format(str(node.id)))

        # Check if parent node limits the value rank
        setNodeValueRankRecursive(node.parent, nodeset)


        if node.parent.valueRank is not None:
            node.valueRank = node.parent.valueRank
        else:
            raise RuntimeError("Node {}: the ValueRank of the parent node is None.".format(str(node.id)))


def generateCommonVariableCode(node, nodeset):
    code = []
    codeCleanup = []
    codeGlobal = []

    if node.valueRank is None:
        # Set the constrained value rank from the type/parent node
        setNodeValueRankRecursive(node, nodeset)
        code.append("/* Value rank inherited */")

    code.append("attr.valueRank = %d;" % node.valueRank)
    if node.valueRank > 0:
        code.append("attr.arrayDimensionsSize = %d;" % node.valueRank)
        code.append("UA_UInt32 arrayDimensions[{}];".format(node.valueRank))
        if len(node.arrayDimensions) == node.valueRank:
            for idx, v in enumerate(node.arrayDimensions):
                code.append("arrayDimensions[{}] = {};".format(idx, int(str(v))))
        else:
            for dim in range(0, node.valueRank):
                code.append("arrayDimensions[{}] = 0;".format(dim))
        code.append("attr.arrayDimensions = &arrayDimensions[0];")

    if node.dataType is None:
        # Inherit the datatype from the HasTypeDefinition reference, as stated in the OPC UA Spec:
        # 6.4.2
        # "Instances inherit the initial values for the Attributes that they have in common with the
        # TypeDefinitionNode from which they are instantiated, with the exceptions of the NodeClass and
        # NodeId."
        setNodeDatatypeRecursive(node, nodeset)
        code.append("/* DataType inherited */")

    dataTypeNode = nodeset.getBaseDataType(nodeset.getDataTypeNode(node.dataType))

    if dataTypeNode is None:
        raise RuntimeError("Cannot get BaseDataType for dataType : " + str(node.dataType) + " of node " + node.browseName.name + " " + str(node.id))

    code.append("attr.dataType = %s;" % generateNodeIdCode(node.dataType))

    if dataTypeNode.isEncodable():
        if node.value is not None:
            [code1, codeCleanup1, codeGlobal1] = generateValueCode(node.value, nodeset.nodes[node.id], nodeset)
            code += code1
            codeCleanup += codeCleanup1
            codeGlobal += codeGlobal1
            # #1978 Variant arrayDimensions are only required to properly decode multidimensional arrays
            # (valueRank > 1) from data stored as one-dimensional array of arrayLength elements.
            # One-dimensional arrays are already completely defined by arraylength attribute so setting
            # also arrayDimensions, even if not explicitly forbidden, can confuse clients
            if node.valueRank is not None and node.valueRank > 1 and len(node.arrayDimensions) == node.valueRank and len(node.value.value) > 0:
                numElements = 1
                hasZero = False
                for v in node.arrayDimensions:
                    dim = int(unicode(v))
                    if dim > 0:
                        numElements = numElements * dim
                    else:
                        hasZero = True
                if hasZero == False and len(node.value.value) == numElements:
                    code.append("attr.value.arrayDimensionsSize = attr.arrayDimensionsSize;")
                    code.append("attr.value.arrayDimensions = attr.arrayDimensions;")
    elif node.value is not None:
        code.append("/* Cannot encode the value */")
        logger.warn("Cannot encode dataTypeNode: " + dataTypeNode.browseName.name + " for value of node " + node.browseName.name + " " + str(node.id))

    return [code, codeCleanup, codeGlobal]

def generateVariableNodeCode(node, nodeset):
    code = []
    codeCleanup = []
    codeGlobal = []
    code.append("UA_VariableAttributes attr = UA_VariableAttributes_default;")
    if node.historizing:
        code.append("attr.historizing = true;")
    code.append("attr.minimumSamplingInterval = %f;" % node.minimumSamplingInterval)
    code.append("attr.userAccessLevel = %d;" % node.userAccessLevel)
    code.append("attr.accessLevel = %d;" % node.accessLevel)
    [code1, codeCleanup1, codeGlobal1] = generateCommonVariableCode(node, nodeset)
    code += code1
    codeCleanup += codeCleanup1
    codeGlobal += codeGlobal1

    return [code, codeCleanup, codeGlobal]

def generateVariableTypeNodeCode(node, nodeset):
    code = []
    codeCleanup = []
    codeGlobal = []
    code.append("UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    [code1, codeCleanup1, codeGlobal1] = generateCommonVariableCode(node, nodeset)
    code += code1
    codeCleanup += codeCleanup1
    codeGlobal += codeGlobal1

    return [code, codeCleanup, codeGlobal]

def lowerFirstChar(inputString):
    return inputString[0].lower() + inputString[1:]

def generateExtensionObjectSubtypeCode(node, parent, nodeset, global_var_code, instanceName=None, isArrayElement=False):
    code = [""]
    codeCleanup = [""]

    logger.debug("Building extensionObject for " + str(parent.id))
    logger.debug("Encoding " + str(node.encodingRule))

    parentDataType = nodeset.getDataTypeNode(parent.dataType)
    parentDataTypeName = nodeset.getDataTypeNode(parent.dataType).browseName.name
    if parentDataType.symbolicName is not None and parentDataType.symbolicName.value is not None:
        parentDataTypeName = parentDataType.symbolicName.value

    typeBrowseNode = makeCIdentifier(parentDataTypeName)
    #TODO: review this
    if typeBrowseNode == "NumericRange":
        # in the stack we define a separate structure for the numeric range, but
        # the value itself is just a string
        typeBrowseNode = "String"


    typeString = "UA_" + typeBrowseNode
    if instanceName is None:
        instanceName = generateNodeValueInstanceName(node, parent, 0)
        code.append("UA_STACKARRAY(" + typeString + ", " + instanceName + ", 1);")
    typeArr = nodeset.getDataTypeNode(parent.dataType).typesArray
    typeArrayString = typeArr + "[" + typeArr + "_" + parentDataTypeName.upper() + "]"
    code.append("UA_init({ref}{instanceName}, &{typeArrayString});".format(ref="&" if isArrayElement else "",
                                                                           instanceName=instanceName,
                                                                           typeArrayString=typeArrayString))

    # Assign data to the struct contents
    # Track the encoding rule definition to detect arrays and/or ExtensionObjects
    values = node.value
    if values == None:
        values = []
    for idx,subv in enumerate(values):
        if subv is None:
            continue
        encField = node.encodingRule[idx].name
        encRule = node.encodingRule[idx]
        memberName = makeCIdentifier(lowerFirstChar(encField))

        # Check if this is an array
        accessor = "." if isArrayElement else "->"

        if isinstance(subv, list):
            if len(subv) == 0:
                continue
            logger.info("ExtensionObject contains array")
            memberName = makeCIdentifier(lowerFirstChar(encField))
            encTypeString = "UA_" + subv[0].__class__.__name__
            instanceNameSafe = makeCIdentifier(instanceName)
            code.append("UA_STACKARRAY(" + encTypeString + ", " + instanceNameSafe + "_" + memberName+", {0});".format(len(subv)))
            encTypeArr = nodeset.getDataTypeNode(subv[0].__class__.__name__).typesArray
            encTypeArrayString = encTypeArr + "[" + encTypeArr + "_" + subv[0].__class__.__name__.upper() + "]"
            code.append("UA_init({instanceName}, &{typeArrayString});".format(instanceName=instanceNameSafe + "_" + memberName,
                                                                              typeArrayString=encTypeArrayString))

            for subArrayIdx,val in enumerate(subv):
                code.append(generateNodeValueCode(instanceNameSafe + "_" + memberName + "[" + str(subArrayIdx) + "]",
                                                  val, instanceName,instanceName + "_gehtNed_member", global_var_code, asIndirect=False))
            code.append(instanceName + accessor + memberName + "Size = {0};".format(len(subv)))
            code.append(instanceName + accessor + memberName + " = " + instanceNameSafe+"_"+ memberName+";")
            continue

        logger.debug("Encoding of field " + memberName + " is " + str(subv.encodingRule) + "defined by " + str(encField))
        if not subv.isNone():
            # Some values can be optional
            valueName = instanceName + accessor + memberName
            code.append(generateNodeValueCode(valueName,
                        subv, instanceName,valueName, global_var_code, asIndirect=False, nodeset=nodeset, encRule=encRule))

    if not isArrayElement:
        code.append("UA_Variant_setScalar(&attr.value, " + instanceName + ", &" + typeArrayString + ");")

    return [code, codeCleanup]

def getTypeBrowseName(dataTypeNode):
    typeBrowseName = makeCIdentifier(dataTypeNode.browseName.name)
    #TODO: review this
    if typeBrowseName == "NumericRange":
        # in the stack we define a separate structure for the numeric range, but
        # the value itself is just a string
        typeBrowseName = "String"
    return typeBrowseName

def getTypesArrayForValue(nodeset, value):
    typeNode = nodeset.getNodeByBrowseName(value.__class__.__name__)
    if typeNode is None or value.isInternal:
        typesArray = "UA_TYPES"
    else:
        typesArray = typeNode.typesArray
    typeName = makeCIdentifier(value.__class__.__name__.upper())
    return "&" + typesArray + "[" + typesArray + "_" + typeName + "]"


def isArrayVariableNode(node, parentNode):
    return parentNode.valueRank is not None and (parentNode.valueRank != -1 and (parentNode.valueRank >= 0
                                       or (len(node.value) > 1
                                           and (parentNode.valueRank != -2 or parentNode.valueRank != -3))))

def generateValueCode(node, parentNode, nodeset, bootstrapping=True):
    code = []
    codeCleanup = []
    codeGlobal = []
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
        return ["", "", ""]
    if not isinstance(node.value[0], Value):
        return ["", "", ""]

    dataTypeNode = nodeset.getDataTypeNode(parentNode.dataType)

    if isArrayVariableNode(node, parentNode):
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if isinstance(node.value[0], DiagnosticInfo):
            logger.warn("Don't know how to print array of DiagnosticInfo in node " + str(parentNode.id))
        elif isinstance(node.value[0], StatusCode):
            logger.warn("Don't know how to print array of StatusCode in node " + str(parentNode.id))
        else:
            if isinstance(node.value[0], ExtensionObject):
                code.append("UA_" + getTypeBrowseName(dataTypeNode) + " " + valueName + "[" + str(len(node.value)) + "];")
                for idx, v in enumerate(node.value):
                    logger.debug("Building extObj array index " + str(idx))
                    instanceName = valueName + "[" + str(idx) + "]"
                    [code1, codeCleanup1] = generateExtensionObjectSubtypeCode(v, parent=parentNode, nodeset=nodeset,
                                                                               global_var_code=codeGlobal, instanceName=instanceName,
                                                                               isArrayElement=True)
                    code = code + code1
                    codeCleanup = codeCleanup + codeCleanup1
            else:
                code.append("UA_" + node.value[0].__class__.__name__ + " " + valueName + "[" + str(len(node.value)) + "];")
                for idx, v in enumerate(node.value):
                    instanceName = generateNodeValueInstanceName(v, parentNode, idx)
                    code.append(generateNodeValueCode(
                        valueName + "[" + str(idx) + "]" , v, instanceName, valueName, codeGlobal))
            code.append("UA_Variant_setArray(&attr.value, &" + valueName +
                        ", (UA_Int32) " + str(len(node.value)) + ", " + "&" +
                        dataTypeNode.typesArray + "["+dataTypeNode.typesArray + "_" + getTypeBrowseName(dataTypeNode).upper() +"]);")
    #scalar value
    else:
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if isinstance(node.value[0], DiagnosticInfo):
            logger.warn("Don't know how to print scalar DiagnosticInfo in node " + str(parentNode.id))
        elif isinstance(node.value[0], StatusCode):
            logger.warn("Don't know how to print scalar StatusCode in node " + str(parentNode.id))
        else:
            # The following strategy applies to all other types, in particular strings and numerics.
            if isinstance(node.value[0], ExtensionObject):
                [code1, codeCleanup1] = generateExtensionObjectSubtypeCode(node.value[0], parent=parentNode, nodeset=nodeset,
                                                                           global_var_code=codeGlobal, isArrayElement=False)
                code = code + code1
                codeCleanup = codeCleanup + codeCleanup1
            instanceName = generateNodeValueInstanceName(node.value[0], parentNode, 0)
            if not node.value[0].isNone() and not(isinstance(node.value[0], ExtensionObject)):
                code.append("UA_" + node.value[0].__class__.__name__ + " *" + valueName + " =  UA_" + node.value[
                    0].__class__.__name__ + "_new();")
                code.append("if (!" + valueName + ") return UA_STATUSCODE_BADOUTOFMEMORY;")
                code.append("UA_" + node.value[0].__class__.__name__ + "_init(" + valueName + ");")
                code.append(generateNodeValueCode("*" + valueName, node.value[0], instanceName, valueName, codeGlobal, asIndirect=True))
                code.append(
                        "UA_Variant_setScalar(&attr.value, " + valueName + ", " +
                        getTypesArrayForValue(nodeset, node.value[0]) + ");")
                if node.value[0].__class__.__name__ == "ByteString":
                    # The data is on the stack, not heap, so we can not delete the ByteString
                    codeCleanup.append("{}->data = NULL;".format(valueName))
                    codeCleanup.append("{}->length = 0;".format(valueName))
                codeCleanup.append("UA_{0}_delete({1});".format(
                    node.value[0].__class__.__name__, valueName))
    return [code, codeCleanup, codeGlobal]

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

def generateSubtypeOfDefinitionCode(node):
    for ref in node.inverseReferences:
        # 45 = HasSubtype
        if ref.referenceType.i == 45:
            return generateNodeIdCode(ref.target)
    return "UA_NODEID_NULL"

def generateNodeCode_begin(node, nodeset, code_global):
    code = []
    codeCleanup = []
    code.append("UA_StatusCode retVal = UA_STATUSCODE_GOOD;")

    # Attributes
    if isinstance(node, ReferenceTypeNode):
        code.extend(generateReferenceTypeNodeCode(node))
    elif isinstance(node, ObjectNode):
        code.extend(generateObjectNodeCode(node))
    elif isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        [code1, codeCleanup1, codeGlobal1] = generateVariableNodeCode(node, nodeset)
        code.extend(code1)
        codeCleanup.extend(codeCleanup1)
        code_global.extend(codeGlobal1)
    elif isinstance(node, VariableTypeNode):
        [code1, codeCleanup1, codeGlobal1] = generateVariableTypeNodeCode(node, nodeset)
        code.extend(code1)
        codeCleanup.extend(codeCleanup1)
        code_global.extend(codeGlobal1)
    elif isinstance(node, MethodNode):
        code.extend(generateMethodNodeCode(node))
    elif isinstance(node, ObjectTypeNode):
        code.extend(generateObjectTypeNodeCode(node))
    elif isinstance(node, DataTypeNode):
        code.extend(generateDataTypeNodeCode(node))
    elif isinstance(node, ViewNode):
        code.extend(generateViewNodeCode(node))
    if node.displayName is not None:
        code.append("attr.displayName = " + generateLocalizedTextCode(node.displayName, alloc=False) + ";")
    if node.description is not None:
        code.append("#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS")
        code.append("attr.description = " + generateLocalizedTextCode(node.description, alloc=False) + ";")
        code.append("#endif")
    if node.writeMask is not None:
        code.append("attr.writeMask = %d;" % node.writeMask)
    if node.userWriteMask is not None:
        code.append("attr.userWriteMask = %d;" % node.userWriteMask)

    # AddNodes call
    code.append("retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_{},".
            format(makeCIdentifier(node.__class__.__name__.upper().replace("NODE" ,""))))
    code.append(generateNodeIdCode(node.id) + ",")
    code.append(generateNodeIdCode(node.parent.id if node.parent else NodeId()) + ",")
    code.append(generateNodeIdCode(node.parentReference.id if node.parent else NodeId()) + ",")
    code.append(generateQualifiedNameCode(node.browseName) + ",")
    if isinstance(node, VariableNode) or isinstance(node, ObjectNode):
        typeDefRef = node.popTypeDef()
        code.append(generateNodeIdCode(typeDefRef.target) + ",")
    else:
        code.append(" UA_NODEID_NULL,")
    code.append("(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_{}ATTRIBUTES],NULL, NULL);".
            format(makeCIdentifier(node.__class__.__name__.upper().replace("NODE" ,""))))
    code.append("if (retVal != UA_STATUSCODE_GOOD) return retVal;")
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
