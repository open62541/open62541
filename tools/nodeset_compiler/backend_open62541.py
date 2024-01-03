#!/usr/bin/env python3
# -*- coding: utf-8 -*-

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
###    Copyright 2021 (c) Wind River Systems, Inc.


from __future__ import print_function
from os.path import basename
import logging
import codecs
import os
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

import sys
if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s

logger = logging.getLogger(__name__)

from datatypes import NodeId
from nodes import *
from nodeset import *
from backend_open62541_nodes import generateNodeCode_begin, generateNodeCode_finish, generateReferenceCode

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
            if not ref.referenceType in relevant_refs:
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
            if not ref.referenceType in relevant_refs:
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
            if not ref.referenceType in relevant_refs:
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
        print(unicode(line), end='\n', file=outfileh)

    def writec(line):
        print(unicode(line), end='\n', file=outfilec)

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

#ifndef %s_H_
#define %s_H_
""" % (outfilebase.upper(), outfilebase.upper()))
    if internal_headers:
        writeh("""
#ifdef UA_ENABLE_AMALGAMATION
# include "open62541.h"

/* The following declarations are in the open62541.c file so here's needed when compiling nodesets externally */

# ifndef UA_INTERNAL //this definition is needed to hide this code in the amalgamated .c file

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

# endif // UA_INTERNAL

#else // UA_ENABLE_AMALGAMATION
# include <open62541/server.h>
#endif

%s
""" % (additionalHeaders))
    else:
        writeh("""
#ifdef UA_ENABLE_AMALGAMATION
# include "open62541.h"
#else
# include <open62541/server.h>
#endif
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
                writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_begin(UA_Server *server, UA_UInt16* ns) {")
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
        writec("}");

        writec("\nstatic UA_StatusCode function_" + outfilebase + "_" + str(functionNumber) + "_finish(UA_Server *server, UA_UInt16* ns) {")

        if isinstance(node, MethodNode) or isinstance(node.parent, MethodNode):
            writec("#ifdef UA_ENABLE_METHODCALLS")
        writec("return " + generateNodeCode_finish(node))
        if isinstance(node, MethodNode) or isinstance(node.parent, MethodNode):
            writec("#else")
            writec("return UA_STATUSCODE_GOOD;")
            writec("#endif /* UA_ENABLE_METHODCALLS */")
        writec("}");

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
    writec("UA_UInt16 ns[" + str(len(nodeset.namespaces)) + "];")
    for i, nsid in enumerate(nodeset.namespaces):
        nsid = nsid.replace("\"", "\\\"")
        writec("ns[" + str(i) + "] = UA_Server_addNamespace(server, \"" + nsid + "\");")

    # Change namespaceIndex from the current namespace,
    # but only if it defines its own data types, otherwise it is not necessary.
    if len(typesArray) > 0:
        typeArr = typesArray[-1]
        if typeArr != "UA_TYPES" and typeArr != "ns0":
            writec("/* Change namespaceIndex from current namespace */")
            writec("#if " + typeArr + "_COUNT" + " > 0")
            writec("for(int i = 0; i < " + typeArr + "_COUNT" + "; i++) {")
            writec(typeArr + "[i]" + ".typeId.namespaceIndex = ns[" + str(nodeset.namespaceMapping[1]) + "];")
            writec(typeArr + "[i]" + ".binaryEncodingId.namespaceIndex = ns[" + str(nodeset.namespaceMapping[1]) + "];")
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
            writec("retVal |= function_{outfilebase}_{idx}_begin(server, ns);". \
                   format(outfilebase=outfilebase, idx=str(i)))
            if i in reftypes_functionNumbers:
                writec("retVal |= function_{outfilebase}_{idx}_finish(server, ns);". \
                       format(outfilebase=outfilebase, idx=str(i)))

        for i in reversed(range(0, functionNumber)):
            if i in reftypes_functionNumbers:
                continue
            writec("retVal |= function_{outfilebase}_{idx}_finish(server, ns);". \
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

