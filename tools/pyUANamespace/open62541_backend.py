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

import string
import logging; logger = logging.getLogger(__name__)

from ua_constants import *
from open62541_backend_nodes import Node_printOpen62541CCode, getCreateStandaloneReference

####################
# Helper Functions #
####################

def substitutePunctuationCharacters(input):
    # No punctuation characters <>!$
    for illegal_char in list(string.punctuation):
        if illegal_char == '_': # underscore is allowed
            continue
        input = input.replace(illegal_char, "_") # map to underscore
    return input

defined_typealiases = []
def getNodeIdDefineString(node):
    extrNs = node.browseName().split(":")
    symbolic_name = ""
    # strip all characters that would be illegal in C-Code
    if len(extrNs) > 1:
        nodename = extrNs[1]
    else:
        nodename = extrNs[0]

    symbolic_name = substitutePunctuationCharacters(nodename)
    if symbolic_name != nodename :
        logger.warn("Substituted characters in browsename for nodeid " + \
                    str(node.id().i) + " while generating C-Code ")
    if symbolic_name in defined_typealiases:
      logger.warn("Typealias definition of " + str(node.id().i) + " is non unique!")
      extendedN = 1
      while (symbolic_name+"_"+str(extendedN) in defined_typealiases):
        logger.warn("Typealias definition of " + str(node.id().i) + " is non unique!")
        extendedN+=1
      symbolic_name = symbolic_name+"_"+str(extendedN)
    defined_typealiases.append(symbolic_name)
    return "#define UA_NS%sID_%s %s" % (node.id().ns, symbolic_name.upper(), node.id().i)

###################
# Generate C Code #
###################

def generateCCode(nodeset, printedExternally=[], supressGenerationOfAttribute=[],
                  outfilename="", high_level_api=False):
    unPrintedNodes = []
    unPrintedRefs  = []
    code = []
    header = []

    # Reorder our nodes to produce a bare minimum of bootstrapping dependencies
    logger.debug("Reordering nodes for minimal dependencies during printing.")
    nodeset.reorderNodesMinDependencies(printedExternally)

    # Populate the unPrinted-Lists with everything we have. Every Time a nodes
    # printfunction is called, it will pop itself and all printed references
    # from these lists.
    for n in nodeset.nodes:
      if not n in printedExternally:
        unPrintedNodes.append(n)
      else:
        logger.debug("Node " + str(n.id()) + " is being ignored.")
    for n in unPrintedNodes:
      for r in n.getReferences():
        if (r.target() != None) and (r.target().id() != None) and (r.parent() != None):
          unPrintedRefs.append(r)
    logger.debug("%d nodes and %d references need to get printed.", len(unPrintedNodes), len(unPrintedRefs))

    # Print the preamble of the generated code
    header.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")
    code.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")
    header.append('#ifndef '+outfilename.upper()+'_H_')
    header.append('#define '+outfilename.upper()+'_H_')
    header.append('#ifdef UA_NO_AMALGAMATION')
    header.append('#include "ua_types.h"')
    if high_level_api:
        header.append('#include "ua_job.h"')
        header.append('#include "ua_server.h"')
    if not high_level_api:
        header.append('#include "server/ua_server_internal.h"')
        header.append('#include "server/ua_nodes.h"')
        header.append('#include "ua_util.h"')
        header.append('#include "ua_types_encoding_binary.h"')
        header.append('#include "ua_types_generated_encoding_binary.h"')
        header.append('#include "ua_transport_generated_encoding_binary.h"')
    header.append('#else')
    header.append('#include "open62541.h"')
    header.append('#define NULL ((void *)0)')
    header.append('#endif')
    code.append('#include "'+outfilename+'.h"')
    code.append("UA_INLINE void "+outfilename+"(UA_Server *server) {")

    # Before printing nodes, we need to request additional namespace arrays from the server
    for nsid in nodeset.namespaceIdentifiers:
      if nsid == 0 or nsid==1:
        continue
      else:
        name =  nodeset.namespaceIdentifiers[nsid]
        name = name.replace("\"","\\\"")
        code.append("UA_Server_addNamespace(server, \"" + name + "\");")

    # Find all references necessary to create the namespace and
    # "Bootstrap" them so all other nodes can safely use these referencetypes whenever
    # they can locate both source and target of the reference.
    logger.debug("Collecting all references used in the namespace.")
    refsUsed = []
    for n in nodeset.nodes:
      # Since we are already looping over all nodes, use this chance to print NodeId defines
      if n.id().ns != 0:
        nc = n.nodeClass()
        if nc != NODE_CLASS_OBJECT and nc != NODE_CLASS_VARIABLE and nc != NODE_CLASS_VIEW:
          header.append(getNodeIdDefineString(n))

      # Now for the actual references...
      for r in n.getReferences():
        # Only print valid references in namespace 0 (users will not want their refs bootstrapped)
        if not r.referenceType() in refsUsed and r.referenceType() != None and r.referenceType().id().ns == 0:
          refsUsed.append(r.referenceType())
    logger.debug("%d reference types are used in the namespace, which will now get bootstrapped.", len(refsUsed))
    for r in refsUsed:
      code.extend(Node_printOpen62541CCode(r, unPrintedNodes, unPrintedRefs, supressGenerationOfAttribute))

    logger.debug("%d Nodes, %d References need to get printed.", len(unPrintedNodes), len(unPrintedRefs))

    if not high_level_api:
        # Note to self: do NOT - NOT! - try to iterate over unPrintedNodes!
        #               Nodes remove themselves from this list when printed.
        logger.debug("Printing all other nodes.")
        for n in nodeset.nodes:
          code.extend(Node_printOpen62541CCode(n, unPrintedNodes, unPrintedRefs, supressGenerationOfAttribute))

        if len(unPrintedNodes) != 0:
          logger.warn("%d nodes could not be translated to code.", len(unPrintedNodes))
        else:
          logger.debug("Printing suceeded for all nodes")

        if len(unPrintedRefs) != 0:
          logger.debug("Attempting to print " + str(len(unPrintedRefs)) + " unprinted references.")
          tmprefs = []
          for r in unPrintedRefs:
            if  not (r.target() not in unPrintedNodes) and not (r.parent() in unPrintedNodes):
              if not isinstance(r.parent(), opcua_node_t):
                logger.debug("Reference has no parent!")
              elif not isinstance(r.parent().id(), opcua_node_id_t):
                logger.debug("Parents nodeid is not a nodeID!")
              else:
                if (len(tmprefs) == 0):
                  code.append("//  Creating leftover references:")
                code.extend(getCreateStandaloneReference(r.parent(), r))
                code.append("")
                tmprefs.append(r)
          # Remove printed refs from list
          for r in tmprefs:
            unPrintedRefs.remove(r)
          if len(unPrintedRefs) != 0:
            logger.warn("" + str(len(unPrintedRefs)) + " references could not be translated to code.")
        else:
          logger.debug("Printing succeeded for all references")
    else:  # Using only High Level API
        already_printed = list(printedExternally)
        while unPrintedNodes:
            node_found = False
            for node in unPrintedNodes:
                for ref in node.getReferences():
                    if ref.referenceType() in already_printed and ref.target() in already_printed:
                        node_found = True
                        code.extend(Node_printOpen62541CCode_HL_API(node, ref, supressGenerationOfAttribute))
                        unPrintedRefs.remove(ref)
                        unPrintedNodes.remove(node)
                        already_printed.append(node)
                        break
            if not node_found:
                logger.critical("no complete code generation with high level API possible; not all nodes will be created")
                code.append("CRITICAL: no complete code generation with high level API possible; not all nodes will be created")
                break
        code.append("// creating references")
        for r in unPrintedRefs:
            code.extend(getCreateStandaloneReference(r.parent(), r))

    # finalizing source and header
    header.append("extern void "+outfilename+"(UA_Server *server);\n")
    header.append("#endif /* "+outfilename.upper()+"_H_ */")
    code.append("} // closing nodeset()")
    return (header,code)
