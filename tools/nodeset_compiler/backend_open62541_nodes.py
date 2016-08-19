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

###########################################
# Extract References with Special Meaning #
###########################################

def extractNodeParent(node):
    """Return a tuple of the most likely (parent, parentReference). The
    parentReference is removed form the inverse references list of the node.

    """
    # TODO What a parent is depends on the node type
    for ref in node.inverseReferences:
        node.inverseReferences.remove(ref)
        return (ref.target, ref.referenceType)
    return (None, None)

def extractNodeType(node):
    """Returns the most likely type of the variable- or objecttype node. The
     isinstanceof reference is removed form the inverse references list of the
     node.

    """
    pass

def extractNodeSuperType(node):
    """Returns the most likely supertype of the variable-, object-, or referencetype
       node. The reference to the supertype is removed from the inverse
       references list of the node.

    """
    pass

#################
# Generate Code #
#################

def generateReferenceCode(reference):
    if reference.isForward:
        return "UA_Server_addReference(server, %s, %s, %s, true);" % \
            (generateNodeIdCode(reference.source), \
             generateNodeIdCode(reference.referenceType), \
             generateExpandedNodeIdCode(reference.target))
    else:
      return "UA_Server_addReference(server, %s, %s, %s, false);" % \
          (generateNodeIdCode(reference.source), \
           generateNodeIdCode(reference.referenceType), \
           generateExpandedNodeIdCode(reference.target))

def generateReferenceTypeNodeCode(node):
    code = []
    code.append("UA_ReferenceTypeAttributes attr;")
    code.append("UA_ReferenceTypeAttributes_init(&attr);")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    if node.symmetric:
        code.append("attr.symmetric  = true;")
    if node.inverseName != "":
        code.append("attr.inverseName  = UA_LOCALIZEDTEXT_ALLOC(\"en_US\", \"%s\");" % \
                    node.inverseName)
    return code;

def generateObjectNodeCode(node):
    code = []
    code.append("UA_ObjectAttributes attr;")
    code.append("UA_ObjectAttributes_init(&attr);")
    if node.eventNotifier:
        code.append("attr.eventNotifier = true;")
    return code;

def generateVariableNodeCode(node):
    code = []
    code.append("UA_VariableAttributes attr;")
    code.append("UA_VariableAttributes_init(&attr);")
    if node.historizing:
        code.append("attr.historizing = true;")
    code.append("attr.minimumSamplingInterval = %f;" % node.minimumSamplingInterval)
    code.append("attr.userAccessLevel = %d;" % node.userAccessLevel)
    code.append("attr.accessLevel = %d;" % node.accessLevel)
    code.append("attr.valueRank = %d;" % node.valueRank)
    # # The variant is guaranteed to exist by SubtypeEarly()
    # code.append(getCodePrintableNodeID(node) + ".value.variant.value = *" + \
    #             getCodePrintableNodeID(node) + "_variant;")
    # code.append(getCodePrintableNodeID(node) + ".valueSource = UA_VALUESOURCE_VARIANT;")
    return code

def generateVariableTypeNodeCode(node):
    code = []
    code.append("UA_VariableTypeAttributes attr;")
    code.append("UA_VariableTypeAttributes_init(&attr);")
    if node.historizing:
        code.append("attr.historizing = true;")
    code.append("attr.valueRank = (UA_Int32)%s;" %str(node.valueRank))
    # # The variant is guaranteed to exist by SubtypeEarly()
    # code.append(getCodePrintableNodeID(node) + ".value.variant.value = *" + \
    #             getCodePrintableNodeID(node) + "_variant;")
    # code.append(getCodePrintableNodeID(node) + ".valueSource = UA_VALUESOURCE_VARIANT;")
    return code

def generateMethodNodeCode(node):
    code = []
    code.append("UA_MethodAttributes attr;")
    code.append("UA_MethodAttributes_init(&attr);")
    if node.executable:
      code.append("attr.executable = true;")
    if node.userExecutable:
      code.append("attr.userExecutable = true;")
    return code

def generateObjectTypeNodeCode(node):
    code = []
    code.append("UA_ObjectTypeAttributes attr;")
    code.append("UA_ObjectTypeAttributes_init(&attr);")
    if node.isAbstract:
      code.append("attr.isAbstract = true;")
    return code

def generateDataTypeNodeCode(node):
    code = []
    code.append("UA_DataTypeAttributes attr;")
    code.append("UA_DataTypeAttributes_init(&attr);")
    if node.isAbstract:
      code.append("attr.isAbstract = true;")
    return code

def generateViewNodeCode(node):
    code = []
    code.append("UA_ViewAttributes attr;")
    code.append("UA_ViewAttributes_init(&attr);")
    if node.containsNoLoops:
      code.append("attr.containsNoLoops = true;")
    code.append("attr.eventNotifier = (UA_Byte)%s;" % str(node.eventNotifier))
    return code

def generateNodeCode(node, supressGenerationOfAttribute, generate_ns0):
    code = []
    code.append("\n{")

    if isinstance(node, ReferenceTypeNode):
        code.extend(generateReferenceTypeNodeCode(node))
    elif isinstance(node, ObjectNode):
        code.extend(generateObjectNodeCode(node))
    elif isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        code.extend(generateVariableNodeCode(node))
    elif isinstance(node, VariableTypeNode):
        code.extend(generateVariableTypeNodeCode(node))
    elif isinstance(node, MethodNode):
        code.extend(generateMethodNodeCode(node))
    elif isinstance(node, ObjectTypeNode):
        code.extend(generateObjectTypeNodeCode(node))
    elif isinstance(node, DataTypeNode):
        code.extend(generateDataTypeNodeCode(node))
    elif isinstance(node, ViewNode):
        code.extend(generateViewNodeCode(node))

    code.append("attr.displayName = " + generateLocalizedTextCode(node.displayName) + ";")
    code.append("attr.description = " + generateLocalizedTextCode(node.description) + ";")
    code.append("attr.writeMask = %d;" % node.writeMask)
    code.append("attr.userWriteMask = %d;" % node.userWriteMask)
    
    if not generate_ns0:
        (parentNode, parentRef) = extractNodeParent(node)
    else:
        (parentNode, parentRef) = (NodeId(), NodeId())

    code.append("UA_Server_add%s(server," % node.__class__.__name__)
    code.append(generateNodeIdCode(node.id) + ",")
    code.append(generateNodeIdCode(parentNode) + ",")
    code.append(generateNodeIdCode(parentRef) + ",")
    code.append(generateQualifiedNameCode(node.browseName) + ",")
    if (isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode)) or isinstance(node, ObjectNode):
        code.append("UA_NODEID_NUMERIC(0,0),") # parent
    code.append("attr,")
    if isinstance(node, MethodNode):
        code.append("NULL, NULL, 0, NULL, 0, NULL, NULL);")
    else:
        code.append("NULL, NULL);")
    code.append("}")
    return "\n".join(code)
