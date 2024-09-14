#!/usr/bin/env python3

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
    generateExpandedNodeIdCode, splitStringLiterals, makeCLiteral
import re
import logging

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
    forwardFlag = "true" if reference.isForward else "false"
    return "retVal |= UA_Server_addReference(server, %s, %s, %s, %s);" % \
        (generateNodeIdCode(reference.source),
         generateNodeIdCode(reference.referenceType),
         generateExpandedNodeIdCode(reference.target),
         forwardFlag)

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
                code_part += " | "
            code_part += "UA_EVENTNOTIFIER_HISTORY_READ"
            is_first = False
        if node.eventNotifier & 8:
            if not is_first:
                code_part += " | "
            code_part += "UA_EVENTNOTIFIER_HISTORY_WRITE"
            is_first = False
        code_part += ";"
        code.append(code_part)
    return code

def setNodeValueRankRecursive(node, nodeset):
    if not isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        raise RuntimeError(f"Node {str(node.id)}: ValueRank can only be set for VariableNode and VariableTypeNode")

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
            raise RuntimeError(f"Node {str(node.id)}: the ValueRank of the parent node is None.")
    else:
        if node.parent is None:
            raise RuntimeError(f"Node {str(node.id)}: does not have a parent. Probably the parent node was blacklisted?")

        # Check if parent node limits the value rank
        setNodeValueRankRecursive(node.parent, nodeset)

        if node.parent.valueRank is not None:
            node.valueRank = node.parent.valueRank
        else:
            raise RuntimeError(f"Node {str(node.id)}: the ValueRank of the parent node is None.")

def getXMLText(elem):
    out = []
    for c in elem.childNodes:
        if c.nodeType == minidom.Node.TEXT_NODE:
            out += c.nodeValue
        else:
            if c.nodeType == minidom.Node.ELEMENT_NODE:
                if c.childNodes.length == 0:
                    out += "<" + c.nodeName + "/>"
                else:
                    out += "<" + c.nodeName + ">"
                    cs = getChildXML(c)
                    out += cs
                    out += "</" + c.nodeName + ">"
    return "".join(out)

def generateCommonVariableCode(node, nodeset):
    code = []

    # Print the ValueRank
    if node.valueRank is None:
        # Set the constrained value rank from the type/parent node
        setNodeValueRankRecursive(node, nodeset)
        code.append("/* Value rank inherited */")
    code.append("attr.valueRank = %d;" % node.valueRank)

    # Print the ArrayDimensions
    if node.valueRank > 0:
        code.append("attr.arrayDimensionsSize = %d;" % node.valueRank)
        code.append(f"UA_UInt32 arrayDimensions[{node.valueRank}];")
        if len(node.arrayDimensions) == node.valueRank:
            for idx, v in enumerate(node.arrayDimensions):
                code.append(f"arrayDimensions[{idx}] = {int(str(v))};")
        else:
            for dim in range(0, node.valueRank):
                code.append(f"arrayDimensions[{dim}] = 0;")
        code.append("attr.arrayDimensions = &arrayDimensions[0];")

    # Print the DataType
    code.append("attr.dataType = %s;" % generateNodeIdCode(node.dataType))

    # Print the value as the raw XML and parse as a variant
    if node.value:
        escapedXml = splitStringLiterals(makeCLiteral(getXMLText(node.value)))
        code.append("UA_ByteString xmlVal = UA_STRING(%s);" % escapedXml)
        code.append("retVal |= UA_decodeXml(&xmlVal, &attr.value, &UA_TYPES[UA_TYPES_VARIANT], NULL);")

    return code

def generateVariableNodeCode(node, nodeset):
    code = []
    code.append("UA_VariableAttributes attr = UA_VariableAttributes_default;")
    if node.historizing:
        code.append("attr.historizing = true;")
    code.append("attr.minimumSamplingInterval = %f;" % node.minimumSamplingInterval)
    code.append("attr.userAccessLevel = %d;" % node.userAccessLevel)
    code.append("attr.accessLevel = %d;" % node.accessLevel)
    code += generateCommonVariableCode(node, nodeset)
    return code

def generateVariableTypeNodeCode(node, nodeset):
    code = []
    code.append("UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    code += generateCommonVariableCode(node, nodeset)
    return code

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
        code.extend(generateVariableNodeCode(node, nodeset))
    elif isinstance(node, VariableTypeNode):
        code.extend(generateVariableTypeNodeCode(node, nodeset))
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
        code.append("\n#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS\n")
        code.append("attr.description = " + generateLocalizedTextCode(node.description, alloc=False) + ";")
        code.append("\n#endif\n")
    if node.writeMask is not None:
        code.append("attr.writeMask = %d;" % node.writeMask)
    if node.userWriteMask is not None:
        code.append("attr.userWriteMask = %d;" % node.userWriteMask)

    # AddNodes call
    addnode = []
    addnode.append("retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_{},".
            format(makeCIdentifier(node.__class__.__name__.upper().replace("NODE" ,""))))
    addnode.append(generateNodeIdCode(node.id) + ",")
    addnode.append(generateNodeIdCode(node.parent.id if node.parent else NodeId()) + ",")
    addnode.append(generateNodeIdCode(node.parentReference.id if node.parent else NodeId()) + ",")
    addnode.append(generateQualifiedNameCode(node.browseName) + ",")
    if isinstance(node, VariableNode) or isinstance(node, ObjectNode):
        typeDefRef = node.popTypeDef()
        addnode.append(generateNodeIdCode(typeDefRef.target) + ",")
    else:
        addnode.append(" UA_NODEID_NULL,")
    addnode.append("(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_{}ATTRIBUTES],NULL, NULL);".
            format(makeCIdentifier(node.__class__.__name__.upper().replace("NODE" ,""))))
    code.append("".join(addnode))

    code.extend(codeCleanup)
    return "\n".join(code)

def generateNodeCode_finish(node):
    code = []
    if isinstance(node, MethodNode):
        code.append("UA_Server_addMethodNode_finish(server, ")
        code.append(generateNodeIdCode(node.id))
        code.append(", NULL, 0, NULL, 0, NULL);")
    else:
        code.append("UA_Server_addNode_finish(server, ")
        code.append(generateNodeIdCode(node.id))
        code.append(");")
    return "".join(code)
