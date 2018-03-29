#!/usr/bin/env/python
# -*- coding: utf-8 -*-

###
### Authors:
### - Chris Iatrou (ichrispa@core-vector.net)
### - Julius Pfrommer
### - Stefan Profanter (profanter@fortiss.org)
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

from __future__ import print_function
import string
from collections import deque
from os.path import basename
import logging
import codecs
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

logger = logging.getLogger(__name__)

from constants import *
from nodes import *
from nodeset import *
from backend_open62541_nodes import generateNodeCode_begin, generateNodeCode_finish, generateReferenceCode

##############
# Sort Nodes #
##############

# Select the references that shall be generated after this node in the ordering
# If both nodes of the reference are hidden we assume that the references between
# those nodes are already setup. Still print if only the target node is hidden,
# because we need that reference.
def selectPrintRefs(nodeset, L, node):
    printRefs = []
    for ref in node.references:
        targetnode = nodeset.nodes[ref.target]
        if node.hidden and targetnode.hidden:
            continue
        if not targetnode.hidden and not targetnode in L:
            continue
        printRefs.append(ref)
    for ref in node.inverseReferences:
        targetnode = nodeset.nodes[ref.target]
        if node.hidden and targetnode.hidden:
            continue
        if not targetnode.hidden and not targetnode in L:
            continue
        printRefs.append(ref)
    return printRefs

def addTypeRef(nodeset, type_refs, dataTypeId, referencedById):
    if not dataTypeId in type_refs:
        type_refs[dataTypeId] = [referencedById]
    else:
        type_refs[dataTypeId].append(referencedById)


def reorderNodesMinDependencies(nodeset):
    # Kahn's algorithm
    # https://algocoding.wordpress.com/2015/04/05/topological-sorting-python/

    relevant_types = nodeset.getRelevantOrderingReferences()

    # determine in-degree
    in_degree = {u.id: 0 for u in nodeset.nodes.values()}
    dataType_refs = {}
    hiddenCount = 0
    for u in nodeset.nodes.values():  # of each node
        if u.hidden:
            hiddenCount += 1
            continue
        hasTypeDef = None
        for ref in u.references:
            if ref.referenceType.i == 40:
                hasTypeDef = ref.target
            elif (ref.referenceType in relevant_types and ref.isForward) and not nodeset.nodes[ref.target].hidden:
                in_degree[ref.target] += 1
        if hasTypeDef is not None and not nodeset.nodes[hasTypeDef].hidden:
            # we cannot print the node u because it first needs the variable type node
            in_degree[u.id] += 1

        if isinstance(u, VariableNode) and u.dataType is not None:
            dataTypeNode = nodeset.getDataTypeNode(u.dataType)
            if dataTypeNode is not None and not dataTypeNode.hidden:
                # we cannot print the node u because it first needs the data type node
                in_degree[u.id] += 1
                # to be able to decrement the in_degree count, we need to store it here
                addTypeRef(nodeset, dataType_refs,dataTypeNode.id, u.id)

    # collect nodes with zero in-degree
    Q = deque()
    for id in in_degree:
        if in_degree[id] == 0:
            # print referencetypenodes first
            n = nodeset.nodes[id]
            if isinstance(n, ReferenceTypeNode):
                Q.append(nodeset.nodes[id])
            else:
                Q.appendleft(nodeset.nodes[id])

    L = []  # list for order of nodes
    while Q:
        u = Q.pop()  # choose node of zero in-degree
        # decide which references to print now based on the ordering
        u.printRefs = selectPrintRefs(nodeset, L, u)
        if u.hidden:
            continue

        L.append(u)  # and 'remove' it from graph

        if isinstance(u, DataTypeNode):
            # decrement all the nodes which depend on this datatype
            if u.id in dataType_refs:
                for n in dataType_refs[u.id]:
                    if not nodeset.nodes[n].hidden:
                        in_degree[n] -= 1
                    if in_degree[n] == 0:
                        Q.append(nodeset.nodes[n])
                del dataType_refs[u.id]

        for ref in u.inverseReferences:
            if ref.referenceType.i == 40:
                if not nodeset.nodes[ref.target].hidden:
                    in_degree[ref.target] -= 1
                if in_degree[ref.target] == 0:
                    Q.append(nodeset.nodes[ref.target])

        for ref in u.references:
            if (ref.referenceType in relevant_types and ref.isForward):
                if not nodeset.nodes[ref.target].hidden:
                    in_degree[ref.target] -= 1
                if in_degree[ref.target] == 0:
                    Q.append(nodeset.nodes[ref.target])

    if len(L) + hiddenCount != len(nodeset.nodes.values()):
        stillOpen = ""
        for id in in_degree:
            if in_degree[id] == 0:
                continue
            node = nodeset.nodes[id]
            stillOpen += node.browseName.name + "/" + str(node.id) + " = " + str(in_degree[id]) + "\r\n"
        raise Exception("Node graph is circular on the specified references. Still open nodes:\r\n" + stillOpen)
    return L

###################
# Generate C Code #
###################

def generateOpen62541Code(nodeset, outfilename, generate_ns0=False, internal_headers=False, typesArray=[], max_string_length=0):
    outfilebase = basename(outfilename)
    # Printing functions
    outfileh = codecs.open(outfilename + ".h", r"w+", encoding='utf-8')
    outfilec = StringIO()

    def writeh(line):
        print(unicode(line), end='\n', file=outfileh)

    def writec(line):
        print(unicode(line), end='\n', file=outfilec)

    additionalHeaders = ""
    if len(typesArray) > 0:
        for arr in set(typesArray):
            if arr == "UA_TYPES":
                continue
            additionalHeaders += """#include "%s_generated.h"\n""" % arr.lower()

    # Print the preamble of the generated code
    writeh("""/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#ifndef %s_H_
#define %s_H_
""" % (outfilebase.upper(), outfilebase.upper()))
    if internal_headers:
        writeh("""
#ifdef UA_NO_AMALGAMATION
# include "ua_server.h"
# include "ua_types_encoding_binary.h"
#else
# include "open62541.h"

/* The following declarations are in the open62541.c file so here's needed when compiling nodesets externally */

# ifndef UA_Nodestore_remove //this definition is needed to hide this code in the amalgamated .c file

typedef UA_StatusCode (*UA_exchangeEncodeBuffer)(void *handle, UA_Byte **bufPos,
                                                 const UA_Byte **bufEnd);

UA_StatusCode
UA_encodeBinary(const void *src, const UA_DataType *type,
                UA_Byte **bufPos, const UA_Byte **bufEnd,
                UA_exchangeEncodeBuffer exchangeCallback,
                void *exchangeHandle) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

UA_StatusCode
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst,
                const UA_DataType *type, size_t customTypesSize,
                const UA_DataType *customTypes) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

size_t
UA_calcSizeBinary(void *p, const UA_DataType *type);

const UA_DataType *
UA_findDataTypeByBinary(const UA_NodeId *typeId);

# endif // UA_Nodestore_remove

#endif

%s
""" % (additionalHeaders))
    else:
        writeh("""
#include "open62541.h"
""")
    writeh("""
#ifdef __cplusplus
extern "C" {
#endif
    
extern UA_StatusCode %s(UA_Server *server);

#ifdef __cplusplus
}
#endif

#endif /* %s_H_ */""" % \
           (outfilebase, outfilebase.upper()))

    writec("""/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "%s.h"
""" % (outfilebase))

    parentrefs = getSubTypesOf(nodeset, nodeset.getNodeByBrowseName("HierarchicalReferences"))
    parentrefs = list(map(lambda x: x.id, parentrefs))

    # Loop over the sorted nodes
    logger.info("Reordering nodes for minimal dependencies during printing")
    sorted_nodes = reorderNodesMinDependencies(nodeset)
    logger.info("Writing code for nodes and references")
    
    functionNumber = 0

    for node in sorted_nodes:
        # Print node
        if not node.hidden:
            writec("\n/* " + str(node.displayName) + " - " + str(node.id) + " */")
            code = generateNodeCode_begin(node, nodeset, max_string_length, generate_ns0, parentrefs)
            if code is None:
                writec("/* Ignored. No parent */")
                nodeset.hide_node(node.id)
                continue
            else:
                writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_begin(UA_Server *server, UA_UInt16* ns) {\n")
                if isinstance(node, MethodNode):
                    writec("#ifdef UA_ENABLE_METHODCALLS")
                writec(code)

        # Print inverse references leading to this node
        for ref in node.printRefs:
            writec(generateReferenceCode(ref))
            
        writec("return retVal;")
        if isinstance(node, MethodNode):
            writec("#else")
            writec("return UA_STATUSCODE_GOOD;")
            writec("#endif /* UA_ENABLE_METHODCALLS */")
        writec("}");

        writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_finish(UA_Server *server, UA_UInt16* ns) {\n")
        if isinstance(node, MethodNode):
            writec("#ifdef UA_ENABLE_METHODCALLS")
        code = generateNodeCode_finish(node)
        writec("return " + code)
        if isinstance(node, MethodNode):
            writec("#else")
            writec("return UA_STATUSCODE_GOOD;")
            writec("#endif /* UA_ENABLE_METHODCALLS */")
        writec("}");

        functionNumber = functionNumber + 1

    writec("""
UA_StatusCode %s(UA_Server *server) {
UA_StatusCode retVal = UA_STATUSCODE_GOOD;""" % (outfilebase))
    # Generate namespaces (don't worry about duplicates)
    writec("/* Use namespace ids generated by the server */")
    writec("UA_UInt16 ns[" + str(len(nodeset.namespaces)) + "];")
    for i, nsid in enumerate(nodeset.namespaces):
        nsid = nsid.replace("\"", "\\\"")
        writec("ns[" + str(i) + "] = UA_Server_addNamespace(server, \"" + nsid + "\");")

    for i in range(0, functionNumber):
        writec("retVal |= function_" + outfilebase + "_" + str(i) + "_begin(server, ns);")


    for i in reversed(range(0, functionNumber)):
        writec("retVal |= function_" + outfilebase + "_" + str(i) + "_finish(server, ns);")

    writec("return retVal;\n}")
    outfileh.close()
    fullCode = outfilec.getvalue()
    outfilec.close()

    outfilec = codecs.open(outfilename + ".c", r"w+", encoding='utf-8')
    outfilec.write(fullCode)
    outfilec.close()
