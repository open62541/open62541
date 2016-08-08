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

from __future__ import print_function
import string
from collections import deque
import logging; logger = logging.getLogger(__name__)

from constants import *
from nodes import *
from backend_open62541_nodes import Node_printOpen62541CCode, getCreateStandaloneReference

##############
# Sort Nodes #
##############

hassubtype = NodeId("ns=0;i=45")
def getSubTypesOf(nodeset, node):
  re = [node]
  for ref in node.references: 
    if ref.referenceType == hassubtype and ref.isForward:
      re = re + getSubTypesOf(nodeset, ref.target)
  return re

def reorderNodesMinDependencies(nodeset):
    #Kahn's algorithm
    #https://algocoding.wordpress.com/2015/04/05/topological-sorting-python/
    
    relevant_types = [nodeset.getNodeByBrowseName("HierarchicalReferences"),
                      nodeset.getNodeByBrowseName("HasComponent")]
    
    temp = []
    for t in relevant_types:
        temp = temp + getSubTypesOf(nodeset, t)
    relevant_types = map(lambda x: x.id, temp)

    # determine in-degree
    in_degree = { u.id : 0 for u in nodeset.nodes.values() }
    for u in nodeset.nodes.values(): # of each node
        for ref in u.references:
            if(ref.referenceType in relevant_types and ref.isForward):
                in_degree[ref.target] += 1
    
    # collect nodes with zero in-degree
    Q = deque()
    for id in in_degree:
      if in_degree[id] == 0:
        Q.appendleft(nodeset.nodes[id])
 
    L = []     # list for order of nodes
    while Q:
      u = Q.pop()          # choose node of zero in-degree
      L.append(u)          # and 'remove' it from graph
      for ref in u.references:
        if(ref.referenceType in relevant_types and ref.isForward):
         in_degree[ref.target] -= 1
         if in_degree[ref.target] == 0:
           Q.appendleft(nodeset.nodes[ref.target])
    if len(L) != len(nodeset.nodes.values()):
      raise Exception("Node graph is circular on the specified references")
    return L

###################
# Generate C Code #
###################

def generateOpen62541Code(nodeset, outfilename, supressGenerationOfAttribute=[]):
    # Printing functions
    outfileh = open(outfilename + ".h", r"w+")
    outfilec = open(outfilename + ".c", r"w+")
    def writeh(line):
        print(line, end='\n', file=outfileh)
    def writec(line):
        print(line, end='\n', file=outfilec)

    # Print the preamble of the generated code
    writeh("""/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#ifndef %s_H_
#define %s_H_

#ifdef UA_NO_AMALGAMATION
#include "ua_types.h"
#include "ua_job.h"
#include "ua_server.h"
#else
#include "open62541.h"
#define NULL ((void *)0)
#endif
    
extern void %s(UA_Server *server);

#endif /* %s_H_ */""" % \
           (outfilename.upper(), outfilename.upper(), \
            outfilename, outfilename.upper()))

    writec("""/* WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "%s.h"

void %s(UA_Server *server) {""" % (outfilename, outfilename))

    # Generate anmespaces (don't worry about duplicates)
    for nsid in nodeset.namespaces:
      nsid = nsid.replace("\"","\\\"")
      writec("UA_Server_addNsidspace(server, \"" + nsid + "\");")

    # Loop over the sorted nodes
    logger.info("Reordering nodes for minimal dependencies during printing")
    sorted_nodes = reorderNodesMinDependencies(nodeset)
    logger.info("Writing code for nodes and references")
    for node in sorted_nodes:
        # Print node
        if not node.hidden:
            writec(Node_printOpen62541CCode(node, supressGenerationOfAttribute))

        # Print inverse references leading to this node
        for ref in node.inverseReferences:
            if not ref.hidden:
                writec(getCreateStandaloneReference(ref))

    # finalizing source and header
    writec("} // closing nodeset()")
    outfileh.close()
    outfilec.close()
