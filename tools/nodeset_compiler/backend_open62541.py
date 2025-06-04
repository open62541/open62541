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

from datatypes import NodeId
from nodes import *
from nodeset import *

import re
from os.path import basename
import codecs
import os
from io import StringIO

import logging
logger = logging.getLogger(__name__)

# Strip invalid characters to create valid C identifiers (variable names etc):
def makeCIdentifier(value):
    keywords = frozenset(["double", "int", "float", "char"])
    sanitized = re.sub(r'[^\w]', '', value)
    if sanitized in keywords:
        return "_" + sanitized
    else:
        return sanitized

# Escape C strings:
def makeCLiteral(value):
    return re.sub(r'(?<!\\)"', r'\\"', value.replace('\\', r'\\').replace('"', r'\"').replace('\n', r'\n').replace('\r', r'\r'))

def splitStringLiterals(value, splitLength=500):
    """
    Split a string literal longer than splitLength into smaller literals.
    E.g. "Some very long text" will be split into "Some ver" "y long te" "xt"
    On VS2008 there is a maximum allowed length of a single string literal.
    """
    value = value.strip()
    if len(value) < splitLength or splitLength == 0:
        return "\"" + re.sub(r'(?<!\\)"', r'\\"', value) + "\""
    ret = ""
    tmp = value
    while len(tmp) > splitLength:
        ret += "\"" + tmp[:splitLength].replace('"', r'\"') + "\" "
        tmp = tmp[splitLength:]
    ret += "\"" + re.sub(r'(?<!\\)"', r'\\"', tmp) + "\" "
    return ret

def generateLocalizedTextCode(value, alloc=False):
    if value.text is None:
        value.text = ""
    vt = makeCLiteral(value.text)
    return "UA_LOCALIZEDTEXT{}(\"{}\", {})".format("_ALLOC" if alloc else "", '' if value.locale is None else value.locale, splitStringLiterals(vt))

def generateQualifiedNameCode(value, alloc=False,):
    vn = makeCLiteral(value.name)
    return "UA_QUALIFIEDNAME{}(UA_NamespaceMapping_local2Remote(nsMapping, {}), {})".format("_ALLOC" if alloc else "", str(value.ns), splitStringLiterals(vn))

def generateGuidCode(value):
    if isinstance(value, str):
        return f"UA_GUID(\"{value}\")"
    if not value or len(value) != 5:
        return "UA_GUID_NULL"
    else:
        return "UA_GUID(\"{}\")".format('-'.join(value))

def generateNodeIdCode(value):
    if not value:
        return "UA_NODEID_NUMERIC(0, 0)"
    if value.i != None:
        return f"UA_NODEID_NUMERIC(UA_NamespaceMapping_local2Remote(nsMapping, {value.ns}), {value.i}LU)"
    elif value.s != None:
        v = makeCLiteral(value.s)
        return f"UA_NODEID_STRING(UA_NamespaceMapping_local2Remote(nsMapping, {value.ns}), \"{v}\")"
    elif value.g != None:
        return "UA_NODEID_GUID(UA_NamespaceMapping_local2Remote(nsMapping, {}), {})".format(value.ns, generateGuidCode(value.gAsString()))
    raise Exception(str(value) + " NodeID generation for bytestring NodeIDs not supported")

def generateExpandedNodeIdCode(value):
    if value.i != None:
        return "UA_EXPANDEDNODEID_NUMERIC(UA_NamespaceMapping_local2Remote(nsMapping, {}), {}LU)".format(value.ns, str(value.i))
    elif value.s != None:
        vs = makeCLiteral(value.s)
        return "UA_EXPANDEDNODEID_STRING(UA_NamespaceMapping_local2Remote(nsMapping, {}), \"{}\")".format(value.ns, vs)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

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
        code.append(f"UA_UInt32 arrayDimensions[{node.valueRank}];")
        if len(node.arrayDimensions) == node.valueRank:
            for idx, v in enumerate(node.arrayDimensions):
                code.append(f"arrayDimensions[{idx}] = {int(str(v))};")
        else:
            for dim in range(0, node.valueRank):
                code.append(f"arrayDimensions[{dim}] = 0;")
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
        raise RuntimeError("Cannot get BaseDataType for dataType : " + \
                           str(node.dataType) + " of node " + \
                           node.browseName.name + " " + str(node.id))

    code.append("attr.dataType = %s;" % generateNodeIdCode(node.dataType))

    if node.value:
        code.append("#ifdef UA_ENABLE_XML_ENCODING")
        xmlenc = [makeCLiteral(line) for line in node.value.toxml().splitlines()]
        line_lengths = [len(line.lstrip()) for line in node.value.toxml().splitlines()] # length without the C escaping
        xmlLength = sum(line_lengths)
        xmlenc = [(" " * (len(line) - len(line.lstrip()))) + "\"" + line.lstrip() + "\"" for line in xmlenc]
        if xmlLength < 30000:
            outxml = "\n".join(xmlenc)
            code.append(f"UA_String xmlValue = UA_STRING({outxml});")
        else:
            # For MSVC, split large strings into smaller pieces and reassemble
            code.append(f"UA_String xmlValue = UA_BYTESTRING_NULL;")
            code.append(f"retVal |= UA_ByteString_allocBuffer(&xmlValue, {xmlLength});")
            code.append(f"if(retVal == UA_STATUSCODE_GOOD) {{")
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
            code.append(f"}}")
            codeCleanup.append("#ifdef UA_ENABLE_XML_ENCODING")
            codeCleanup.append("UA_String_clear(&xmlValue);")
            codeCleanup.append("#endif /* UA_ENABLE_XML_ENCODING */")

        code.append("""UA_DecodeXmlOptions opts;
memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
opts.unwrapped = true;
opts.namespaceMapping = nsMapping;
opts.customTypes = UA_Server_getConfig(server)->customDataTypes;
retVal |= UA_decodeXml(&xmlValue, &attr.value, &UA_TYPES[UA_TYPES_VARIANT], &opts);""")
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

# Kahn's algorithm: https://algocoding.wordpress.com/2015/04/05/topological-sorting-python/
def sortNodes(nodeset):
    # reverse hastypedefinition references to treat only forward references
    hasTypeDef = NodeId("ns=0;i=40")
    for u in nodeset.nodes.values():
        for ref in u.references:
            if ref.referenceType == hasTypeDef:
                ref.isForward = not ref.isForward

    # Only hierarchical types...
    relevant_refs = nodeset.getRelevantOrderingReferences()

    # determine in-degree of unfulfilled references
    L = [node for node in nodeset.nodes.values() if node.hidden]  # ordered list of nodes
    R = {node.id: node for node in nodeset.nodes.values() if not node.hidden} # remaining nodes
    in_degree = {id: 0 for id in R.keys()}
    for u in R.values(): # for each node
        for ref in u.references:
            if ref.referenceType not in relevant_refs:
                continue
            if nodeset.nodes[ref.target].hidden:
                continue
            if ref.isForward:
                continue
            in_degree[u.id] += 1

    # Print ReferenceType and DataType nodes first. They may be required even
    # though there is no reference to them. For example if the referencetype is
    # used in a reference, it must exist. A Variable node may point to a
    # DataTypeNode in the datatype attribute and not via an explicit reference.

    Q = [node for node in R.values() if in_degree[node.id] == 0 and
         (isinstance(node, ReferenceTypeNode) or isinstance(node, DataTypeNode))]
    while Q:
        u = Q.pop() # choose node of zero in-degree and 'remove' it from graph
        L.append(u)
        del R[u.id]

        for ref in sorted(u.references, key=lambda r: str(r.target)):
            if ref.referenceType not in relevant_refs:
                continue
            if nodeset.nodes[ref.target].hidden:
                continue
            if not ref.isForward:
                continue
            in_degree[ref.target] -= 1
            if in_degree[ref.target] == 0:
                Q.append(R[ref.target])

    # Order the remaining nodes
    Q = [node for node in R.values() if in_degree[node.id] == 0]
    while Q:
        u = Q.pop() # choose node of zero in-degree and 'remove' it from graph
        L.append(u)
        del R[u.id]

        for ref in sorted(u.references, key=lambda r: str(r.target)):
            if ref.referenceType not in relevant_refs:
                continue
            if nodeset.nodes[ref.target].hidden:
                continue
            if not ref.isForward:
                continue
            in_degree[ref.target] -= 1
            if in_degree[ref.target] == 0:
                Q.append(R[ref.target])

    # reverse hastype references
    for u in nodeset.nodes.values():
        for ref in u.references:
            if ref.referenceType == hasTypeDef:
                ref.isForward = not ref.isForward

    if len(L) != len(nodeset.nodes.values()):
        print(len(L))
        stillOpen = ""
        for id in in_degree:
            if in_degree[id] == 0:
                continue
            node = nodeset.nodes[id]
            stillOpen += node.browseName.name + "/" + str(node.id) + " = " + str(in_degree[id]) + \
                                                                         " " + str(node.references) + "\r\n"
        raise Exception("Node graph is circular on the specified references. Still open nodes:\r\n" + stillOpen)
    return L

###################
# Generate C Code #
###################

def generateOpen62541Code(nodeset, outfilename, internal_headers=False, typesArray=[]):
    outfilebase = basename(outfilename)
    # Printing functions
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
    writeh("""
_UA_BEGIN_DECLS

extern UA_StatusCode %s(UA_Server *server);

_UA_END_DECLS

#endif /* %s_H_ */""" % \
           (outfilebase, outfilebase.upper()))

    writec("""/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "%s.h"
""" % (outfilebase))

    # Loop over the sorted nodes
    logger.info("Reordering nodes for minimal dependencies during printing")
    sorted_nodes = sortNodes(nodeset)
    logger.info("Writing code for nodes and references")
    functionNumber = 0

    printed_ids = set()
    reftypes_functionNumbers = list()
    for node in sorted_nodes:
        printed_ids.add(node.id)

        if not node.hidden:
            writec("\n/* " + str(node.displayName) + " - " + str(node.id) + " */")
            code_global = []
            code = generateNodeCode_begin(node, nodeset, code_global)
            if code is None:
                writec("/* Ignored. No parent */")
                nodeset.hide_node(node.id)
                continue
            else:
                if len(code_global) > 0:
                    writec("\n".join(code_global))
                    writec("\n")
                writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_begin(UA_Server *server, UA_NamespaceMapping *nsMapping) {")
                if isinstance(node, MethodNode) or isinstance(node.parent, MethodNode):
                    writec("#ifdef UA_ENABLE_METHODCALLS")
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

        if isinstance(node, MethodNode) or isinstance(node.parent, MethodNode):
            writec("#else")
            writec("return UA_STATUSCODE_GOOD;")
            writec("#endif /* UA_ENABLE_METHODCALLS */")
        writec("}")

        writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_finish(UA_Server *server, UA_NamespaceMapping *nsMapping) {")

        if isinstance(node, MethodNode) or isinstance(node.parent, MethodNode):
            writec("#ifdef UA_ENABLE_METHODCALLS")
        writec("return " + generateNodeCode_finish(node))
        if isinstance(node, MethodNode) or isinstance(node.parent, MethodNode):
            writec("#else")
            writec("return UA_STATUSCODE_GOOD;")
            writec("#endif /* UA_ENABLE_METHODCALLS */")
        writec("}")

        # ReferenceTypeNodes have to be _finished immediately. The _begin phase
        # of other nodes might depend on the subtyping information of the
        # referencetype to be complete.
        if isinstance(node, ReferenceTypeNode):
            reftypes_functionNumbers.append(functionNumber)

        functionNumber = functionNumber + 1

    # Load generated types
    for arr in typesArray:
        if arr == "UA_TYPES":
            continue
        writec("\nstatic UA_DataTypeArray custom" + arr + " = {")
        writec("    NULL,")
        writec("    " + arr + "_COUNT,")
        writec("    " + arr + ",")
        writec("    UA_FALSE\n};")

    writec("""
UA_StatusCode %s(UA_Server *server) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;""" % (outfilebase))

    # Generate namespaces (don't worry about duplicates)
    writec("/* Use namespace ids generated by the server */")
    writec("UA_UInt16 ns[" + str(len(nodeset.namespaces)+1) + "];")
    for i, nsid in enumerate(nodeset.namespaces):
        nsid = nsid.replace("\"", "\\\"")
        writec("ns[" + str(i) + "] = UA_Server_addNamespace(server, \"" + nsid + "\");")

    # Write the list of ns mappings
    maxns = max(nodeset.namespaceMapping.keys()) + 1
    if maxns < 2:
        maxns = 2
    mapping = [0] * maxns
    mapping[1] = 1 # default
    for k, v in nodeset.namespaceMapping.items():
        mapping[k] = v
    writec(f"UA_UInt16 nsMappingTable[{len(mapping)}]" + " = {" + ", ".join(map(lambda x: f"ns[{x}]", mapping)) + "};")

    # Create the namespace mapping
    writec("UA_NamespaceMapping nsMapping;")
    writec("memset(&nsMapping, 0, sizeof(UA_NamespaceMapping));")
    writec("nsMapping.local2remote = ns;")
    writec(f"nsMapping.local2remoteSize = {len(nodeset.namespaces)};")
    writec("nsMapping.remote2local = nsMappingTable;")
    writec(f"nsMapping.remote2localSize = {len(mapping)};")

    # Change namespaceIndex from the current namespace,
    # but only if it defines its own data types, otherwise it is not necessary.
    if len(typesArray) > 0:
        typeArr = typesArray[-1]
        # Build the name of the TypeArray to compare if the current nodeset defines data types.
        currentTypeArr = '_'.join(outfilebase.upper().split('_')[1:-1])
        if typeArr != "UA_TYPES" and typeArr != "ns0" and typeArr == "UA_TYPES_"+currentTypeArr:
            writec("/* Change namespaceIndex from current namespace */")
            writec("#if " + typeArr + "_COUNT" + " > 0")
            writec("for(int i = 0; i < " + typeArr + "_COUNT" + "; i++) {")
            writec(typeArr + "[i]" + ".typeId.namespaceIndex = ns[" + str(len(nodeset.namespaces)-1) + "];")
            writec(typeArr + "[i]" + ".binaryEncodingId.namespaceIndex = ns[" + str(len(nodeset.namespaces)-1) + "];")
            writec("}")
            writec("#endif")

    # Add generated types to the server
    writec("\n/* Load custom datatype definitions into the server */")
    for arr in typesArray:
        if arr == "UA_TYPES":
            continue
        writec("if(" + arr + "_COUNT > 0) {")
        writec("custom" + arr + ".next = UA_Server_getConfig(server)->customDataTypes;")
        writec("UA_Server_getConfig(server)->customDataTypes = &custom" + arr + ";\n")
        writec("}")

    if functionNumber > 0:
        for i in range(0, functionNumber):
            writec("retVal |= function_{outfilebase}_{idx}_begin(server, &nsMapping);". \
                   format(outfilebase=outfilebase, idx=str(i)))
            if i in reftypes_functionNumbers:
                writec("retVal |= function_{outfilebase}_{idx}_finish(server, &nsMapping);". \
                       format(outfilebase=outfilebase, idx=str(i)))

        for i in reversed(range(0, functionNumber)):
            if i in reftypes_functionNumbers:
                continue
            writec("retVal |= function_{outfilebase}_{idx}_finish(server, &nsMapping);". \
                   format(outfilebase=outfilebase, idx=str(i)))

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

