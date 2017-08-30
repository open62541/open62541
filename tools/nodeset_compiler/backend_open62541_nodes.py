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

###########################################
# Extract References with Special Meaning #
###########################################

import logging

logger = logging.getLogger(__name__)

def extractNodeParent(node, parentrefs):
    """Return a tuple of the most likely (parent, parentReference). The
    parentReference is removed form the inverse references list of the node.

    """
    for ref in node.inverseReferences:
        if ref.referenceType in parentrefs:
            node.inverseReferences.remove(ref)
            if ref in node.printRefs:
                node.printRefs.remove(ref)
            return (ref.target, ref.referenceType)
    return None, None

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
    code.append("UA_ReferenceTypeAttributes attr;")
    code.append("UA_ReferenceTypeAttributes_init(&attr);")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    if node.symmetric:
        code.append("attr.symmetric  = true;")
    if node.inverseName != "":
        code.append("attr.inverseName  = UA_LOCALIZEDTEXT_ALLOC(\"\", \"%s\");" % \
                    node.inverseName)
    return code

def generateObjectNodeCode(node):
    code = []
    code.append("UA_ObjectAttributes attr;")
    code.append("UA_ObjectAttributes_init(&attr);")
    if node.eventNotifier:
        code.append("attr.eventNotifier = true;")
    return code

def generateVariableNodeCode(node, nodeset):
    code = []
    code.append("UA_VariableAttributes attr;")
    code.append("UA_VariableAttributes_init(&attr);")
    if node.historizing:
        code.append("attr.historizing = true;")
    code.append("attr.minimumSamplingInterval = %f;" % node.minimumSamplingInterval)
    code.append("attr.userAccessLevel = %d;" % node.userAccessLevel)
    code.append("attr.accessLevel = %d;" % node.accessLevel)
    code.append("attr.valueRank = %d;" % node.valueRank)
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
                    code += generateValueCode(node.value, nodeset.nodes[node.id], nodeset)
                else:
                    code += generateValueCodeDummy(dataTypeNode, nodeset.nodes[node.id], nodeset)
    return code

def generateVariableTypeNodeCode(node, nodeset):
    code = []
    code.append("UA_VariableTypeAttributes attr;")
    code.append("UA_VariableTypeAttributes_init(&attr);")
    if node.historizing:
        code.append("attr.historizing = true;")
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
                    code += generateValueCode(node.value, nodeset.nodes[node.id], nodeset)
                else:
                    code += generateValueCodeDummy(dataTypeNode, nodeset.nodes[node.id], nodeset)
    return code

def generateSubTypeValueCode(node, instanceName, asIndirect=True):
    if type(node) in [Boolean, Byte, SByte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double]:
        return "(UA_" + node.__class__.__name__ + ") " + str(node.value)
    elif type(node) == String:
        return "UA_STRING_ALLOC(\"" + node.value.encode('utf-8') + "\")"
    elif type(node) == XmlElement:
        return "UA_XMLELEMENT_ALLOC(\"" + node.value.encode('utf-8') + "\")"
    elif type(node) == ByteString:
        outs = re.sub(r"[\r\n]+","", node.value).replace('"', r'\"')
        logger.debug("Encoded Bytestring: " + outs)
        #      bs = bs.decode('base64')
        #      outs = ""
        #      for s in bs:
        #        outs = outs + hex(ord(s)).upper().replace("0X", "\\x")
        code = "UA_STRING_ALLOC(\"" + outs + "\")"
        return code
    elif type(node) == LocalizedText:
        if asIndirect == True:
            return "UA_LOCALIZEDTEXT_ALLOC(\"" + str(node.locale) + "\", \"" + \
                   str(node.text.encode('utf-8')) + "\")"
        else:
            return "UA_LOCALIZEDTEXT(\"" + str(node.locale) + "\", \"" + str(node.text.encode('utf-8')) + "\")"
    elif type(node) == NodeId:
        if node.value == None:
            return "UA_NODEID_NUMERIC(0,0)"
        nodeId = node.value.id
        if nodeId.i != None:
            return "UA_NODEID_NUMERIC(" + str(nodeId.ns) + ", " + str(nodeId.i) + ")"
        elif nodeId.s != None:
            return "UA_NODEID_STRING(" + str(nodeId.ns) + ", " + str(nodeId.s) + ")"
        elif nodeId.b != None:
            logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
            return "UA_NODEID_NUMERIC(0,0)"
        elif nodeId.g != None:
            logger.debug("NodeID Generation macro for guids has not been implemented.")
            return "UA_NODEID_NUMERIC(0,0)"
        return "UA_NODEID_NUMERIC(0,0)"
    elif type(node) == ExpandedNodeId:
        if node.value == None:
            return "UA_EXPANDEDNODID_NUMERIC(0,0)"
        nodeId = node.value.id
        if nodeId.i != None:
            return "UA_EXPANDEDNODID_NUMERIC(" + str(nodeId.ns) + ", " + str(nodeId.i) + ")"
        elif nodeId.s != None:
            return "UA_EXPANDEDNODID_STRING(" + str(nodeId.ns) + ", " + str(nodeId.s) + ")"
        elif nodeId.b != None:
            logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
            return "UA_EXPANDEDNODID_NUMERIC(0,0)"
        elif nodeId.g != None:
            logger.debug("NodeID Generation macro for guids has not been implemented.")
            return "UA_EXPANDEDNODID_NUMERIC(0,0)"
        return "UA_EXPANDEDNODID_NUMERIC(0,0)"
    elif type(node) == DateTime:
        epoch = datetime.datetime.utcfromtimestamp(0)
        mSecsSinceEpoch = (node.value - epoch).total_seconds() * 1000.0
        return "( (" + str(mSecsSinceEpoch) + "f * UA_MSEC_TO_DATETIME) + UA_DATETIME_UNIX_EPOCH)"
    elif type(node) == QualifiedName:
        code = "UA_QUALIFIEDNAME_ALLOC(" + str(node.ns) + ", \"" + node.name.encode('utf-8') + "\")"
        return code
    elif type(node) == StatusCode:
        raise Exception("generateSubTypeInstantiationCode for type " + node.__class__.name + " not implemented")
    elif type(node) == DiagnosticInfo:
        raise Exception("generateSubTypeInstantiationCode for type " + node.__class__.name + " not implemented")
    elif type(node) == Guid:
        raise Exception("generateSubTypeInstantiationCode for type " + node.__class__.name + " not implemented")
    elif type(node) == ExtensionObject:
        if asIndirect == False:
            return "*" + str(instanceName)
        return str(instanceName)

def generateSubtypeStructureCode(node, parent, nodeset, recursionDepth=0, arrayIndex=0):
    code = [""]

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
                        generateSubTypeValueCode(subv, instanceName, asIndirect=False) + ";")
        else:
            if isinstance(subv, list):
                # this is an array
                code.append(instanceName + "_struct." + subv.alias + "Size = " + str(len(subv)) + ";")
                code.append(
                    instanceName + "_struct." + subv.alias + " = (UA_" + subv.__class__.__name__ +
                    " *) UA_malloc(sizeof(UA_" + subv.__class__.__name__ + ")*" + str(
                        len(subv)) + ");")
                logger.debug("Encoding included array of " + str(len(subv)) + " values.")
                for subvidx in range(0, len(subv)):
                    subvv = subv[subvidx]
                    logger.debug("  " + str(subvidx) + " " + str(subvv))
                    code.append(instanceName + "_struct." + subv.alias + "[" + str(
                        subvidx) + "] = " + generateSubTypeValueCode(subvv, instanceName, asIndirect=True) + ";")
                code.append("}")
            else:
                code.append(instanceName + "_struct." + subv.alias + "Size = 1;")
                code.append(
                    instanceName + "_struct." + subv.alias + " = (UA_" + subv.__class__.__name__ +
                    " *) UA_malloc(sizeof(UA_" + subv.__class__.__name__ + "));")
                code.append(instanceName + "_struct." + subv.alias + "[0]  = " +
                            generateSubTypeValueCode(subv, instanceName, asIndirect=True) + ";")

    # Allocate some memory
    code.append("UA_ExtensionObject *" + instanceName + " =  UA_ExtensionObject_new();")
    code.append(instanceName + "->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;")
    #if parent.dataType.ns == 0:

    binaryEncodingId = nodeset.getBinaryEncodingIdForNode(parent.dataType)
    code.append(
        instanceName + "->content.encoded.typeId = UA_NODEID_NUMERIC(" + str(binaryEncodingId.ns) + ", " +
        str(binaryEncodingId.i) + ");")
    code.append(
        "if(UA_ByteString_allocBuffer(&" + instanceName + "->content.encoded.body, 65000) != UA_STATUSCODE_GOOD) {}")

    # Encode each value as a bytestring seperately.
    code.append("UA_Byte *pos" + instanceName + " = " + instanceName + "->content.encoded.body.data;")
    code.append("const UA_Byte *end" + instanceName + " = &" + instanceName + "->content.encoded.body.data[65000];")
    encFieldIdx = 0
    code.append("{")
    code.append("UA_StatusCode retval = UA_STATUSCODE_GOOD;")
    for subv in node.value:
        encField = node.encodingRule[encFieldIdx]
        encFieldIdx = encFieldIdx + 1
        if encField[2] == 0:
            code.append(
                "retval |= UA_encodeBinary(&" + instanceName + "_struct." + subv.alias + ", &UA_TYPES[UA_TYPES_" +
                subv.__class__.__name__.upper() + "], &pos" + instanceName + ", &end" + instanceName + ", NULL, NULL);")
        else:
            if isinstance(subv, list):
                for subvidx in range(0, len(subv)):
                    code.append("retval |= UA_encodeBinary(&" + instanceName + "_struct." + subv.alias + "[" +
                                str(subvidx) + "], &UA_TYPES[UA_TYPES_" + subv.__class__.__name__.upper() + "], &pos" +
                                instanceName + ", &end" + instanceName + ", NULL, NULL);")
            else:
                code.append(
                    "retval |= UA_encodeBinary(&" + instanceName + "_struct." + subv.alias + "[0], &UA_TYPES[UA_TYPES_" +
                    subv.__class__.__name__.upper() + "], &pos" + instanceName + ", &end" + instanceName + ", NULL, NULL);")

    code.append("}")
    # Reallocate the memory by swapping the 65k Bytestring for a new one
    code.append("size_t " + instanceName + "_encOffset = (uintptr_t)(" +
                "pos" + instanceName + "-" + instanceName + "->content.encoded.body.data);")
    code.append(instanceName + "->content.encoded.body.length = " + instanceName + "_encOffset;")
    code.append("UA_Byte *" + instanceName + "_newBody = (UA_Byte *) UA_malloc(" + instanceName + "_encOffset );")
    code.append("memcpy(" + instanceName + "_newBody, " + instanceName + "->content.encoded.body.data, " +
                instanceName + "_encOffset);")
    code.append("UA_Byte *" + instanceName + "_oldBody = " + instanceName + "->content.encoded.body.data;")
    code.append(instanceName + "->content.encoded.body.data = " + instanceName + "_newBody;")
    code.append("UA_free(" + instanceName + "_oldBody);")
    code.append("")
    return code


def generateValueCodeDummy(dataTypeNode, parentNode, nodeset, bootstrapping=True):
    code = []
    valueName = generateNodeIdPrintable(parentNode) + "_variant_DataContents"

    type = "UA_TYPES[UA_TYPES_" + dataTypeNode.browseName.name.upper() + "]"

    code.append("void *" + valueName + " = UA_alloca(" + type + ".memSize);")
    code.append("UA_init(" + valueName + ", &" + type + ");")
    code.append("UA_Variant_setScalar(&attr.value, " + valueName + ", &" + type + ");")

    return code

def generateValueCode(node, parentNode, nodeset, bootstrapping=True):
    code = []
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
        return ""
    if not isinstance(node.value[0], Value):
        return ""

    if parentNode.valueRank != -1 and (parentNode.valueRank >= 0
                                       or (len(node.value) > 1
                                           and (parentNode.valueRank != -2 or parentNode.valueRank != -3))):
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_GUID:
            logger.warn("Don't know how to print array of GUID in node " + str(parentNode.id))
        elif node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_DATETIME:
            logger.warn("Don't know how to print array of DateTime in node " + str(parentNode.id))
        elif node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_DIAGNOSTICINFO:
            logger.warn("Don't know how to print array of DiagnosticInfo in node " + str(parentNode.id))
        elif node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_STATUSCODE:
            logger.warn("Don't know how to print array of StatusCode in node " + str(parentNode.id))
        else:
            if node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
                for idx, v in enumerate(node.value):
                    logger.debug("Building extObj array index " + str(idx))
                    code = code + generateSubtypeStructureCode(v, parent=parentNode, nodeset=nodeset, arrayIndex=idx)
            # code.append("attr.value.type = &UA_TYPES[UA_TYPES_" + node.value[0].__class__.__name__.upper() + "];")
            code.append("UA_" + node.value[0].__class__.__name__ + " " + valueName + "[" + str(len(node.value)) + "];")
            if node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
                for idx, v in enumerate(node.value):
                    logger.debug("Printing extObj array index " + str(idx))
                    instanceName = generateNodeValueInstanceName(v, parentNode, 0, idx)
                    code.append(
                        valueName + "[" + str(idx) + "] = " +
                        generateSubTypeValueCode(v, instanceName, asIndirect=False) + ";")
                    # code.append("UA_free(&" +valueName + "[" + str(idx) + "]);")
            else:
                for idx, v in enumerate(node.value):
                    instanceName = generateNodeValueInstanceName(v, parentNode, 0, idx)
                    code.append(
                        valueName + "[" + str(idx) + "] = " + generateSubTypeValueCode(v, instanceName) + ";")
            code.append("UA_Variant_setArray( &attr.value, &" + valueName +
                        ", (UA_Int32) " + str(len(node.value)) + ", &UA_TYPES[UA_TYPES_" + node.value[
                            0].__class__.__name__.upper() + "]);")
    else:
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_GUID:
            logger.warn("Don't know how to print scalar GUID in node " + str(parentNode.id))
        elif node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_DATETIME:
            logger.warn("Don't know how to print scalar DateTime in node " + str(parentNode.id))
        elif node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_DIAGNOSTICINFO:
            logger.warn("Don't know how to print scalar DiagnosticInfo in node " + str(parentNode.id))
        elif node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_STATUSCODE:
            logger.warn("Don't know how to print scalar StatusCode in node " + str(parentNode.id))
        else:
            # The following strategy applies to all other types, in particular strings and numerics.
            if node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
                code = code + generateSubtypeStructureCode(node.value[0], parent=parentNode, nodeset=nodeset)
            # code.append("attr.value.type = &UA_TYPES[UA_TYPES_" + node.value[0].__class__.__name__.upper() + "];")
            instanceName = generateNodeValueInstanceName(node.value[0], parentNode, 0, 0)
            if node.value[0].numericRepresentation == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
                code.append("UA_" + node.value[0].__class__.__name__ + " *" + valueName + " = " +
                            generateSubTypeValueCode(node.value[0], instanceName) + ";")
                code.append(
                    "UA_Variant_setScalar( &attr.value, " + valueName + ", &UA_TYPES[UA_TYPES_" + node.value[
                        0].__class__.__name__.upper() + "]);")

                # FIXME: There is no membership definition for extensionObjects generated in this function.
                # code.append("UA_" + node.value[0].__class__.__name__ + "_deleteMembers(" + valueName + ");")
            else:
                if bootstrapping == True:
                    code.append("UA_Variant* " + generateNodeIdPrintable(parentNode) + "_variant = UA_Variant_new();")
                code.append("UA_" + node.value[0].__class__.__name__ + " *" + valueName + " =  UA_" + node.value[
                    0].__class__.__name__ + "_new();")
                code.append("*" + valueName + " = " + generateSubTypeValueCode(node.value[0], instanceName) + ";")
                if bootstrapping == False:
                    code.append(
                        "UA_Variant_setScalar( &attr.value, " + valueName + ", &UA_TYPES[UA_TYPES_" + node.value[
                            0].__class__.__name__.upper() + "]);")
                else:
                    code.append(
                        "UA_Variant_setScalar( " + generateNodeIdPrintable(
                            parentNode) + "_variant, " + valueName + ", &UA_TYPES[UA_TYPES_" + node.value[
                            0].__class__.__name__.upper() + "]);")
                    # code.append("UA_" + node.value[0].__class__.__name__ + "_deleteMembers(" + valueName + ");")
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

def generateTypeDefinitionCode(node):
    for ref in node.references:
        # 40 = HasTypeDefinition
        if ref.referenceType.i == 40:
            return generateNodeIdCode(ref.target)
    return "UA_NODEID_NULL"

def generateSubtypeOfDefinitionCode(node):
    for ref in node.inverseReferences:
        # 45 = HasSubtype
        if ref.referenceType.i == 45:
            return generateNodeIdCode(ref.target)
    return "UA_NODEID_NULL"

def generateNodeCode(node, supressGenerationOfAttribute, generate_ns0, parentrefs, nodeset):
    code = []
    code.append("{")

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

    code.append("attr.displayName = " + generateLocalizedTextCode(node.displayName) + ";")
    code.append("attr.description = " + generateLocalizedTextCode(node.description) + ";")
    code.append("attr.writeMask = %d;" % node.writeMask)
    code.append("attr.userWriteMask = %d;" % node.userWriteMask)

    # a variable type node always needs a parent node, otherwise adding it to the nodestore will fail
    if not generate_ns0:
        (parentNode, parentRef) = extractNodeParent(node, parentrefs)
        if parentNode is None or parentRef is None:
            return None
    else:
        (parentNode, parentRef) = (NodeId(), NodeId())

    code.append("retVal |= UA_Server_add%s(server," % node.__class__.__name__)
    code.append(generateNodeIdCode(node.id) + ",")
    code.append(generateNodeIdCode(parentNode) + ",")
    code.append(generateNodeIdCode(parentRef) + ",")
    code.append(generateQualifiedNameCode(node.browseName) + ",")
    if isinstance(node, VariableTypeNode):
        # we need the HasSubtype reference
        code.append(generateSubtypeOfDefinitionCode(node) + ",")
    elif isinstance(node, VariableNode) or isinstance(node, ObjectNode):
        code.append(generateTypeDefinitionCode(node) + ",")
    code.append("attr,")
    if isinstance(node, MethodNode):
        code.append("NULL, 0, NULL, 0, NULL, NULL, NULL);")
    else:
        code.append("NULL, NULL);")
    code.append("}\n")
    return "\n".join(code)
