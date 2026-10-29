#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
###    Copyright 2018 (c) Jannis Volker
###    Copyright 2018 (c) Ralph Lange
###    Copyright 2019 (c) Andrea Minosu
###    Copyright 2021 (c) Wind River Systems, Inc.

from .datatypes import NodeId
from .nodes import *
from .nodeset import *
from .backend_open62541_datatypes import CGenerator, makeCIdentifier

import re
import base64
from os.path import basename
import codecs
import os
from io import StringIO

import logging
logger = logging.getLogger(__name__)

# Escape C strings:
def makeCLiteral(value):
    if value is None:
        return ""
    return value.replace('\\', r'\\').replace('"', r'\"').replace('\n', r'\n').replace('\r', r'\r')

def splitStringLiterals(value, splitLength=500):
    """
    Split a string literal longer than splitLength into smaller literals.
    E.g. "Some very long text" will be split into "Some ver" "y long te" "xt"
    On VS2008 there is a maximum allowed length of a single string literal.
    """
    # Input is already C-escaped. Split only between complete escape tokens.
    if splitLength <= 0:
        return '"' + value + '"'
    pieces = []
    start = 0
    while len(value) - start > splitLength:
        end = start + splitLength
        backslashes = 0
        while end - backslashes > start and value[end - backslashes - 1] == "\\":
            backslashes += 1
        if backslashes % 2:
            end -= 1
        if end == start:
            end += 2  # Keep even a single escaped character intact.
        pieces.append(value[start:end])
        start = end
    pieces.append(value[start:])
    return " ".join('"' + piece + '"' for piece in pieces)

def generateLocalizedTextCode(value, alloc=False):
    locale = "NULL" if value.locale is None else splitStringLiterals(makeCLiteral(value.locale))
    text = "NULL" if value.text is None else splitStringLiterals(makeCLiteral(value.text))
    return "UA_LOCALIZEDTEXT{}({}, {})".format("_ALLOC" if alloc else "", locale, text)

def generateQualifiedNameCode(value, alloc=False,):
    if value.name is None:
        value.name = ""
    vn = makeCLiteral(value.name)
    return "UA_QUALIFIEDNAME{}(UA_NamespaceMapping_local2Remote(nsMapping, {}), {})".format("_ALLOC" if alloc else "", str(value.ns), splitStringLiterals(vn))

def generateGuidCode(value):
    if isinstance(value, str):
        return f"UA_GUID(\"{value}\")"
    if not value or len(value) != 5:
        return "UA_GUID_NULL"
    return "UA_GUID(\"{}\")".format('-'.join(value))

def generateNodeIdCode(value):
    if not value:
        return "UA_NODEID_NUMERIC(0, 0)"
    if value.i is not None:
        return f"UA_NODEID_NUMERIC(UA_NamespaceMapping_local2Remote(nsMapping, {value.ns}), {value.i}LU)"
    if value.s is not None:
        v = makeCLiteral(value.s)
        return f"UA_NODEID_STRING(UA_NamespaceMapping_local2Remote(nsMapping, {value.ns}), \"{v}\")"
    if value.g is not None:
        return "UA_NODEID_GUID(UA_NamespaceMapping_local2Remote(nsMapping, {}), {})".format(value.ns, generateGuidCode(value.gAsString()))
    if value.b is not None:
        data = base64.b64decode(value.b, validate=True)
        literal = "".join("\\%03o" % byte for byte in data)
        return ('nodeIdByteString(UA_NamespaceMapping_local2Remote(nsMapping, %d), %d, "%s")' %
                (value.ns, len(data), literal))
    raise Exception("Invalid NodeId: " + str(value))

def generateExpandedNodeIdCode(value):
    if value.i is not None:
        return "UA_EXPANDEDNODEID_NUMERIC(UA_NamespaceMapping_local2Remote(nsMapping, {}), {}LU)".format(value.ns, str(value.i))
    if value.s is not None:
        vs = makeCLiteral(value.s)
        return "UA_EXPANDEDNODEID_STRING(UA_NamespaceMapping_local2Remote(nsMapping, {}), \"{}\")".format(value.ns, vs)
    return "UA_EXPANDEDNODEID_NODEID(%s)" % generateNodeIdCode(value)

#################
# Generate Code #
#################

def generateNodeIdPrintable(node):
    if isinstance(node.id, NodeId):
        CodePrintable = node.__class__.__name__ + "_" + str(node.id)
    else:
        CodePrintable = node.__class__.__name__ + "_unknown_nid"

    return re.sub('[^0-9a-z_]+', '_', CodePrintable.lower())

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
        code.append("attr.inverseName = %s;" % generateLocalizedTextCode(node.inverseName))
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

def setNodeDatatypeRecursive(node, nodeset):

    if not isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        raise RuntimeError(f"Node {str(node.id)}: DataType can only be set for VariableNode and VariableTypeNode")

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
            # TypeDefinition has no ValueRank, apply XSD schema default for scalar nodes
            # UANodeSet.xsd: <xs:attribute name="ValueRank" type="ValueRank" default="-1"/>
            if len(node.arrayDimensions) == 0:
                node.valueRank = -1
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
            # Parent has no ValueRank, apply XSD schema default for scalar nodes
            # UANodeSet.xsd: <xs:attribute name="ValueRank" type="ValueRank" default="-1"/>
            if len(node.arrayDimensions) == 0:
                node.valueRank = -1
            else:
                raise RuntimeError(f"Node {str(node.id)}: the ValueRank of the parent node is None.")


def generateCommonVariableCode(node, nodeset):
    code = []
    codeCleanup = []
    codeGlobal = []

    rank_omitted = node.valueRank is None
    if rank_omitted:
        if isinstance(node, VariableTypeNode):
            setNodeValueRankRecursive(node, nodeset)
        else:
            # NodeSet XML defaults instances to scalar. Only a decoded array
            # without an explicit rank uses the VariableType's legacy rank.
            node.valueRank = -1
            type_definition = nodeset.getNodeTypeDefinition(node)
            if type_definition is not None:
                setNodeValueRankRecursive(type_definition, nodeset)
                if type_definition.valueRank >= 0:
                    node.valueRank = type_definition.valueRank

    code.append("attr.valueRank = %d;" % node.valueRank)
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

    if isinstance(node.dataType, str) and nodeset.getDataTypeNode(node.dataType) is None:
        node.dataType = None
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
        raise RuntimeError("Cannot get BaseDataType for dataType : " + \
                           str(node.dataType) + " of node " + \
                           node.browseName.name + " " + str(node.id))

    node.dataType = nodeset.getDataTypeNode(node.dataType).id
    code.append("attr.dataType = %s;" % generateNodeIdCode(node.dataType))

    if node.value:
        code.append("#ifdef UA_ENABLE_XML_ENCODING")
        text = node.value.toxml()
        chunks = [text[i:i+1024] for i in range(0, len(text), 1024)]
        xmlenc = ['"' + makeCLiteral(chunk) + '"' for chunk in chunks]
        line_lengths = [len(chunk.encode("utf-8")) for chunk in chunks]
        xmlLength = sum(line_lengths)
        if xmlLength < 30000:
            outxml = "\n".join(xmlenc)
            code.append(f"UA_String xmlValue = UA_STRING({outxml});")
        else:
            # For MSVC, split large strings into smaller pieces and reassemble
            code.append("UA_String xmlValue = UA_BYTESTRING_NULL;")
            code.append(f"retVal |= UA_ByteString_allocBuffer(&xmlValue, {xmlLength});")
            code.append("if(retVal == UA_STATUSCODE_GOOD) {")
            pieces = []
            piece_lengths = []
            curlen = 0
            last = 0
            for i,line in enumerate(xmlenc):
                if i == len(xmlenc) - 1:
                    pieces.append(xmlenc[last:])
                    piece_lengths.append(sum(line_lengths[last:]))
                elif curlen + len(line) > 5000:
                    pieces.append(xmlenc[last:i])
                    piece_lengths.append(sum(line_lengths[last:i]))
                    curlen = 0
                    last = i
                curlen += len(line)
            pos = 0
            for i,p in enumerate(pieces):
                outxml = "\n".join(p)
                code.append(f"    char *buf_{i} = {outxml};")
                code.append(f"    memcpy(xmlValue.data + {pos}, buf_{i}, {piece_lengths[i]});")
                pos += piece_lengths[i]
            code.append("}")
            codeCleanup.append("#ifdef UA_ENABLE_XML_ENCODING")
            codeCleanup.append("UA_String_clear(&xmlValue);")
            codeCleanup.append("#endif /* UA_ENABLE_XML_ENCODING */")

        # XML values retain the namespace numbering of their source file.
        mapping = node.namespaceMapping or {0: 0}
        indices = [mapping[i] for i in range(max(mapping) + 1)]
        code.append("UA_UInt16 valueNamespaces[] = {" + ", ".join(
            "UA_NamespaceMapping_local2Remote(nsMapping, %d)" % i for i in indices) + "};")
        code.append("UA_NamespaceMapping valueMapping = *nsMapping;")
        code.append("valueMapping.remote2local = valueNamespaces;")
        code.append("valueMapping.remote2localSize = %d;" % len(indices))
        code.append("""UA_DecodeXmlOptions opts;
memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
opts.unwrapped = true;
opts.namespaceMapping = &valueMapping;
opts.customTypes = UA_Server_getDataTypes(server);
retVal |= UA_decodeXml(&xmlValue, &attr.value, &UA_TYPES[UA_TYPES_VARIANT], &opts);""")
        # Some companion specs (e.g. IOLink, PNENC, PNRIO) declare ValueRank=1
        # (one-dimensional array) but provide a scalar default value in the XML
        # (e.g. <String> instead of <ListOfString>). Wrap the scalar into a
        # one-element array so that the value passes the server's ValueRank
        # compatibility check.
        if node.valueRank is not None and node.valueRank >= 1:
            # Detect at compile time whether the XML value is scalar by
            # checking if the first child element's tag starts with "ListOf".
            valueIsScalar = True
            for child in node.value.childNodes:
                if child.nodeType == child.ELEMENT_NODE:
                    tag = child.localName or child.tagName.split(":")[-1]
                    if tag.startswith("ListOf"):
                        valueIsScalar = False
                    break
            code.append("if(UA_Variant_isScalar(&attr.value) && attr.value.data != NULL) {")
            code.append('    UA_LOG_WARNING(UA_Server_getConfig(server)->logging,')
            code.append('                  UA_LOGCATEGORY_USERLAND,')
            code.append(f'                  "Node {str(node.id)}: ValueRank={node.valueRank} '
                        f'but the XML value is scalar. '
                        f'Auto-wrapping into a one-element array.");')
            code.append("    attr.value.arrayLength = 1;")
            code.append("}")
            if valueIsScalar:
                logger.warning("Node %s: ValueRank=%s "
                               "but the XML value is scalar. "
                               "Auto-wrapping into a one-element array.",
                               str(node.id), node.valueRank)
        code.append("if(retVal == UA_STATUSCODE_GOOD && attr.value.type &&")
        code.append("   !UA_NodeId_equal(&attr.dataType, &UA_TYPES[UA_TYPES_VARIANT].typeId)) {")
        code.append("    const UA_DataType *target = UA_Server_findDataType(server, &attr.dataType);")
        code.append("    if(!target) {")
        code.append("        UA_NodeId representation = %s;" % generateNodeIdCode(dataTypeNode.id))
        code.append("        target = UA_Server_findDataType(server, &representation);")
        code.append("    }")
        # A null data pointer has no elements, even when its type is known.
        # Keep empty arrays eligible for the datatype-only conversion below.
        code.append("    size_t valuesSize = 0;")
        code.append("    if(attr.value.data)")
        code.append("        valuesSize = UA_Variant_isScalar(&attr.value) ? 1 : attr.value.arrayLength;")
        code.append("    if(target && target->typeKind == UA_DATATYPEKIND_UINT32 && attr.value.type == &UA_TYPES[UA_TYPES_INT32]) {")
        code.append("        UA_Boolean nonnegative = true;")
        code.append("        for(size_t k = 0; k < valuesSize; k++)")
        code.append("            if(((UA_Int32*)attr.value.data)[k] < 0) nonnegative = false;")
        code.append("        if(nonnegative) {")
        code.append("            for(size_t k = 0; k < valuesSize; k++) {")
        code.append("                UA_UInt32 converted = (UA_UInt32)((UA_Int32*)attr.value.data)[k];")
        code.append("                memcpy((UA_Byte*)attr.value.data + k * sizeof(UA_UInt32), &converted, sizeof(converted));")
        code.append("            }")
        code.append("            attr.value.type = target;")
        code.append("        }")
        code.append("    }")
        code.append("    const UA_DataType *actual = attr.value.type;")
        code.append("    if(actual == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT] && valuesSize > 0) {")
        code.append("        UA_ExtensionObject *objects = (UA_ExtensionObject*)attr.value.data;")
        code.append("        for(size_t k = 0; k < valuesSize; k++) {")
        code.append("            if((objects[k].encoding != UA_EXTENSIONOBJECT_DECODED && objects[k].encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||")
        code.append("               (k > 0 && objects[k].content.decoded.type != actual)) { actual = attr.value.type; break; }")
        code.append("            actual = objects[k].content.decoded.type;")
        code.append("        }")
        code.append("    }")
        code.append("    if(target && actual == &UA_TYPES[UA_TYPES_ARGUMENT] &&")
        code.append("       target->typeKind != UA_DATATYPEKIND_STRUCTURE && target->typeKind != UA_DATATYPEKIND_OPTSTRUCT &&")
        code.append("       target->typeKind != UA_DATATYPEKIND_UNION) UA_Variant_clear(&attr.value);")
        code.append("}")
        if node.valueRank > 1 and (node.arrayDimensions or isinstance(node, VariableTypeNode)):
            code.append("if(retVal == UA_STATUSCODE_GOOD && attr.value.arrayLength > 0 && attr.arrayDimensionsSize > 0) {")
            code.append("    UA_Array_delete(attr.value.arrayDimensions, attr.value.arrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);")
            code.append("    attr.value.arrayDimensions = NULL; attr.value.arrayDimensionsSize = 0;")
            code.append("    retVal = UA_Array_copy(attr.arrayDimensions, attr.arrayDimensionsSize, (void**)&attr.value.arrayDimensions, &UA_TYPES[UA_TYPES_UINT32]);")
            code.append("    if(retVal == UA_STATUSCODE_GOOD) attr.value.arrayDimensionsSize = attr.arrayDimensionsSize;")
            code.append("}")
        if node.valueRank == 1:
            # Match the established loader repair for legacy array defaults.
            code.append("if(retVal == UA_STATUSCODE_GOOD && attr.arrayDimensionsSize == 1 &&")
            code.append("   attr.arrayDimensions[0] > 0 && attr.value.arrayLength > attr.arrayDimensions[0])")
            code.append("    attr.arrayDimensions[0] = attr.value.arrayLength <= UA_UINT32_MAX ?")
            code.append("        (UA_UInt32)attr.value.arrayLength : 0;")
        if rank_omitted and not isinstance(node, VariableTypeNode):
            type_definition = nodeset.getNodeTypeDefinition(node)
            if type_definition is not None:
                code.append("if(attr.value.type && !UA_Variant_isScalar(&attr.value))")
                code.append("    (void)UA_Server_readValueRank(server, %s, &attr.valueRank);" %
                            generateNodeIdCode(type_definition.id))
        code.append("#endif /* UA_ENABLE_XML_ENCODING */")

        codeCleanup.append("#ifdef UA_ENABLE_XML_ENCODING")
        codeCleanup.append("UA_Variant_clear(&attr.value);")
        codeCleanup.append("#endif /* UA_ENABLE_XML_ENCODING */")

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

def generateViewNodeCode(node):
    code = []
    code.append("UA_ViewAttributes attr = UA_ViewAttributes_default;")
    if node.containsNoLoops:
        code.append("attr.containsNoLoops = true;")
    code.append("attr.eventNotifier = (UA_Byte)%s;" % str(node.eventNotifier))
    return code

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
        code.append("UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;")
        if node.isAbstract:
            code.append("attr.isAbstract = true;")
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

    code.append('if(retVal != UA_STATUSCODE_GOOD) UA_LOG_ERROR(UA_Server_getConfig(server)->logging,')
    code.append('    UA_LOGCATEGORY_USERLAND, "Cannot add generated node %%N (%%s)", %s, UA_StatusCode_name(retVal));' %
                generateNodeIdCode(node.id))
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

###################
# Generate C Code #
###################

def generateOpen62541Code(nodeset, outfilename, internal_headers=False,
                          typesArray=None, typesOutput=None):
    """Generate a namespace loader and its optional static datatype array."""
    if typesArray is None:
        typesArray = []
    outfilebase = basename(outfilename)
    typesOutname = basename(typesOutput) if typesOutput else outfilebase

    typeParser = nodeset.normalizeDataTypes(typesOutname, bool(typesOutput), typesArray)

    # Generate the public static datatype array directly from the NodeSet XML.
    # This retains the established UA_TYPES_<SPEC> API without routing the
    # definitions through a temporary BSD file.
    if typesOutput:
        # NodeSet namespace indexes are remapped at load time; they are not
        # fixed server indexes supplied through --namespaceMap.
        generator = CGenerator(typeParser, "", typesOutput, internal_headers,
                               False, {"http://opcfoundation.org/UA/": 0})
        generator.write_definitions()

        localTypesArray = "UA_" + typesOutname.upper()
        typesArray = list(typesArray)
        typesArray.append(localTypesArray)

    # Build the namespace source and public header in memory.
    outfileh = codecs.open(outfilename + ".h", r"w+", encoding='utf-8')
    outfilec = StringIO()

    def writeh(line):
        print(line, end='\n', file=outfileh)

    def writec(line):
        print(line, end='\n', file=outfilec)

    additionalHeaders = ""
    if len(typesArray) > 0:
        for arr in typesArray:
            if arr == "UA_TYPES":
                continue
            # remove ua_ prefix if exists
            typeFile = arr.lower()
            typeFile = typeFile[typeFile.startswith("ua_") and len("ua_"):]
            additionalHeaders += """#include "%s_generated.h"\n""" % typeFile

    # Print the preamble of the generated code
    writeh("""/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#ifndef {}_H_
#define {}_H_
""".format(outfilebase.upper(), outfilebase.upper()))
    writeh("""
#include <open62541/server.h>
%s
""" % (additionalHeaders))

    loaderArgs = "UA_Server *server" if outfilebase == "namespace0_generated" else "UA_Server *server, UA_Boolean copy_types"
    writeh("""
_UA_BEGIN_DECLS

/* copy_types creates server-owned descriptors; false binds mutable static
 * arrays to one server. Dependencies must already be loaded. On failure,
 * declared nodes are removed; registered type arrays remain for retry. */
extern UA_StatusCode %s(%s);

_UA_END_DECLS

#endif /* %s_H_ */""" % (outfilebase, loaderArgs, outfilebase.upper()))

    writec("""/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "%s.h"
""" % (outfilebase))

    writec("#include <open62541/internal/generated_types.h>")
    for arr in dict.fromkeys(typesArray):
        if arr != "UA_TYPES":
            typeFile = arr.lower()
            if typeFile.startswith("ua_"):
                typeFile = typeFile[3:]
            writec('#include "' + typeFile + '_generated_internal.h"')

    if any(node.id.b is not None for node in nodeset.nodes.values()):
        writec("""static UA_NodeId
nodeIdByteString(UA_UInt16 nsIndex, size_t length, char *data) {
    UA_NodeId id = UA_NODEID_BYTESTRING(nsIndex, data);
    id.identifier.byteString.length = length;
    return id;
}
""")

    # Loop over the sorted nodes
    logger.info("Writing code for nodes and references")
    functionNumber = 0

    printed_ids = set()
    reftypes_functionNumbers = list()
    functionNodes = []
    datatype_functionNumbers = []
    for node in nodeset.nodes.values():
        printed_ids.add(node.id)

        if not node.hidden:
            writec("\n/* " + str(node.displayName) + " - " + str(node.id) + " */")
            code_global = []
            code = generateNodeCode_begin(node, nodeset, code_global)
            if code is None:
                writec("/* Ignored. No parent */")
                nodeset.hide_node(node.id)
                continue
            if len(code_global) > 0:
                writec("\n".join(code_global))
                writec("\n")
            writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_begin(UA_Server *server, UA_NamespaceMapping *nsMapping) {")
            writec(code)

        # Print inverse references leading to this node
        for ref in node.references:
            if ref.target not in printed_ids:
                continue
            if node.hidden and nodeset.nodes[ref.target].hidden:
                continue
            if node.parent is not None and ref.target == node.parent.id \
                and ref.referenceType == node.parentReference.id:
                # Skip parent reference
                continue
            writec(generateReferenceCode(ref))

        if node.hidden:
            continue

        writec("return retVal;")

        writec("}")

        writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_finish(UA_Server *server, UA_NamespaceMapping *nsMapping) {")

        writec("UA_StatusCode retVal = " + generateNodeCode_finish(node))
        writec('if(retVal != UA_STATUSCODE_GOOD) UA_LOG_ERROR(UA_Server_getConfig(server)->logging,')
        writec('    UA_LOGCATEGORY_USERLAND, "Cannot finish generated node %%N (%%s)", %s, UA_StatusCode_name(retVal));' %
               generateNodeIdCode(node.id))
        writec("return retVal;")
        writec("}")

        # ReferenceTypeNodes have to be _finished immediately. The _begin phase
        # of other nodes might depend on the subtyping information of the
        # referencetype to be complete.
        if isinstance(node, ReferenceTypeNode):
            reftypes_functionNumbers.append(functionNumber)

        functionNodes.append(node)
        if isinstance(node, DataTypeNode):
            datatype_functionNumbers.append(functionNumber)
        functionNumber = functionNumber + 1

    # Only arrays owned by this loader are registered. Dependency arrays are
    # resolved through the receiving server, never appended a second time.
    if typesOutput:
        ownedArrays = [localTypesArray]
    else:
        ownedArrays = list(dict.fromkeys(node.typesArray for node in functionNodes
                                       if isinstance(node, DataTypeNode) and
                                       node.typesArray != "UA_TYPES"))
    sources = list(dict.fromkeys(arr for arr in typesArray if arr != "UA_TYPES"))
    writec("static const UA_String namespaceUris[] = {")
    for uri in nodeset.namespaces:
        writec('    UA_STRING_STATIC("%s"),' % makeCLiteral(uri))
    writec("};")
    writec("static UA_DataTypeArraySource *const datatypeSources[] = {NULL,")
    for arr in sources:
        writec("#if " + arr + "_COUNT > 0")
        writec("    &" + arr + "_SOURCE,")
        writec("#endif")
    writec("};")
    writec("UA_StatusCode %s(%s) {" % (outfilebase, loaderArgs))
    writec("UA_StatusCode retVal = UA_STATUSCODE_GOOD;")
    writec("(void)datatypeSources;")
    if outfilebase != "namespace0_generated":
        writec("(void)copy_types;")
    writec("UA_Boolean started = false;")
    writec("UA_NamespaceMapping nsMapping;")
    writec("memset(&nsMapping, 0, sizeof(nsMapping));")
    writec("UA_UInt16 ns[%d];" % len(nodeset.namespaces))
    maxns = max(nodeset.namespaceMapping.keys(), default=0) + 1
    writec("UA_UInt16 nsMappingTable[%d];" % maxns)
    writec("size_t namespaceUrisSize = 0;")
    for i, uri in enumerate(nodeset.namespaces):
        writec('ns[%d] = UA_Server_addNamespace(server, "%s");' % (i, makeCLiteral(uri)))
        if i > 0:
            writec("if(ns[%d] == 0) { retVal = UA_STATUSCODE_BADOUTOFMEMORY; goto cleanup; }" % i)
        writec("if((size_t)ns[%d] + 1 > namespaceUrisSize) namespaceUrisSize = (size_t)ns[%d] + 1;" % (i, i))
    writec("nsMapping.namespaceUris = (UA_String*)UA_calloc(namespaceUrisSize, sizeof(UA_String));")
    writec("if(!nsMapping.namespaceUris) { retVal = UA_STATUSCODE_BADOUTOFMEMORY; goto cleanup; }")
    writec("nsMapping.namespaceUrisSize = namespaceUrisSize;")
    for i in range(len(nodeset.namespaces)):
        writec("nsMapping.namespaceUris[ns[%d]] = namespaceUris[%d];" % (i, i))
    mapping = [0] * maxns
    for k, v in nodeset.namespaceMapping.items():
        mapping[k] = v
    for i, value in enumerate(mapping):
        writec("nsMappingTable[%d] = ns[%d];" % (i, value))
    writec("nsMapping.local2remote = ns;")
    writec("nsMapping.local2remoteSize = %d;" % len(nodeset.namespaces))
    writec("nsMapping.remote2local = nsMappingTable;")
    writec("nsMapping.remote2localSize = %d;" % maxns)
    if outfilebase != "namespace0_generated":
        writec("/* Reject duplicates before creating any nodes. */")
        for node in functionNodes:
            nodeid = generateNodeIdCode(node.id).replace("nsMapping,", "&nsMapping,")
            writec("{ UA_NodeClass nodeClass; retVal = UA_Server_readNodeClass(server, " + nodeid +
                   ", &nodeClass); if(retVal == UA_STATUSCODE_GOOD) { retVal = UA_STATUSCODE_BADNODEIDEXISTS; goto cleanup; } "
                   "if(retVal != UA_STATUSCODE_BADNODEIDUNKNOWN) goto cleanup; }")
    writec("started = true;")
    def emitBegin(i):
        writec("retVal = function_%s_%d_begin(server, &nsMapping);" % (outfilebase, i))
        writec("if(retVal != UA_STATUSCODE_GOOD) goto cleanup;")
    for i in reftypes_functionNumbers:
        emitBegin(i)
        writec("retVal = function_%s_%d_finish(server, &nsMapping);" % (outfilebase, i))
        writec("if(retVal != UA_STATUSCODE_GOOD) goto cleanup;")
    for i in datatype_functionNumbers:
        emitBegin(i)
    for arr in ownedArrays:
        writec("#if " + arr + "_COUNT > 0")
        writec("retVal = UA_Server_addGeneratedDataTypes(server, &" + arr + "_SOURCE,")
        writec("    sizeof(datatypeSources) / sizeof(datatypeSources[0]) - 1, datatypeSources + 1, copy_types);")
        writec("if(retVal != UA_STATUSCODE_GOOD) goto cleanup;")
        writec("#endif")
    for i in range(functionNumber):
        if i not in reftypes_functionNumbers and i not in datatype_functionNumbers:
            emitBegin(i)
    for i in reversed(range(functionNumber)):
        if i not in reftypes_functionNumbers:
            writec("retVal = function_%s_%d_finish(server, &nsMapping);" % (outfilebase, i))
            writec("if(retVal != UA_STATUSCODE_GOOD) goto cleanup;")
    writec("cleanup:")
    if outfilebase != "namespace0_generated":
        writec("if(started && retVal != UA_STATUSCODE_GOOD) {")
        for node in reversed(functionNodes):
            nodeid = generateNodeIdCode(node.id).replace("nsMapping,", "&nsMapping,")
            writec("    UA_Server_deleteNode(server, " + nodeid + ", true);")
        writec("}")
    else:
        writec("(void)started;")
    writec("UA_free(nsMapping.namespaceUris);")
    writec("return retVal;\n}")
    outfileh.flush()
    os.fsync(outfileh)
    outfileh.close()
    fullCode = outfilec.getvalue()
    outfilec.close()

    outfilec = codecs.open(outfilename + ".c", r"w+", encoding='utf-8')
    outfilec.write(fullCode)
    outfilec.flush()
    os.fsync(outfilec)
    outfilec.close()
