#!/usr/bin/env python3
# -*- coding: utf-8 -*-

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2021 (c) Simon Kueppers, 2pi-Labs GmbH


import logging
import argparse
import sys
import os
from datatypes import NodeId
from lxml import etree
from nodeset import NodeSet
import nodes

# Parse the arguments
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('-e', '--existing',
                    metavar="<existingNodeSetXML>",
                    type=argparse.FileType('rb'),
                    dest="existing",
                    action='append',
                    default=[],
                    help='NodeSet XML files with nodes that are already present on the server.')

parser.add_argument('-x', '--xml',
                    metavar="<nodeSetXML>",
                    type=argparse.FileType('rb'),
                    action='append',
                    dest="infiles",
                    default=[],
                    help='NodeSet XML files with nodes that dependencies shall be resolved for.')

parser.add_argument('-r', '--ref',
                    metavar="<referenceNodeSetXML>",
                    type=argparse.FileType('rb'),
                    dest="refs",
                    action='append',
                    default=[],
                    help='NodeSet XML files where missing dependencies are resolved from.')

parser.add_argument('-u', '--expanded',
                    action='store_true',
                    dest='expanded',
                    default=False,
                    help='Output expanded node ids with a namespace uri')

parser.add_argument('-p', '--pull',
                    action='store_true',
                    dest='pull',
                    default=False,
                    help='Pull in and output missing Nodes from (first) reference XML')

parser.add_argument('-m', '--merge',
                    action='store_true',
                    dest='merge',
                    default=False,
                    help='Merge missing Nodes from reference NodeSet into (first) existing XML')

parser.add_argument('-v', '--verbose', action='count',
                    default=1,
                    help='Make the script more verbose. Can be applied up to 4 times')



args = parser.parse_args()

# Set up logging
# Leave logging output on sys.stderr so that we can output XML data on stdout
logging.basicConfig(stream=sys.stderr)
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
verbosity = 0
if args.verbose:
    verbosity = int(args.verbose)
if verbosity == 1:
    logging.basicConfig(level=logging.ERROR)
elif verbosity == 2:
    logging.basicConfig(level=logging.WARNING)
elif verbosity == 3:
    logging.basicConfig(level=logging.INFO)
elif verbosity >= 4:
    logging.basicConfig(level=logging.DEBUG)
else:
    logging.basicConfig(level=logging.CRITICAL)

# Create a new nodeset. The nodeset name is not significant.
# Parse the XML files
ns = NodeSet()
nsCount = 0
loadedFiles = list()

for xmlfile in args.existing:
    if xmlfile.name in loadedFiles:
        logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
        continue
    loadedFiles.append(xmlfile.name)
    logger.info("Preprocessing (existing) " + str(xmlfile.name))
    ns.addNodeSet(xmlfile, True)
    nsCount +=1
for xmlfile in args.infiles:
    if xmlfile.name in loadedFiles:
        logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
        continue
    loadedFiles.append(xmlfile.name)
    logger.info("Preprocessing " + str(xmlfile.name))
    ns.addNodeSet(xmlfile)
    nsCount +=1

for n in ns.nodes.values():
    if n.sanitize() == False:
        raise Exception("Failed to sanitize node " + str(n))

# Function for walking a tree of nodeIds in the nodeSet and recording a list
# of nodes encountered during the walk
def walkNodes(nodeSet, nodeIds, nodeList=[]):
    for nodeId in nodeIds:
        if nodeId not in nodeSet.nodes:
            # Can not follow unresolved dependency
            continue

        n = nodeSet.nodes[nodeId]
        candidateNodes = []

        # Gather candidateNodes from various node attributes
        if type(n) == nodes.DataTypeNode and n.__isEnum__ == False and n.__isOptionSet__ == False:
            # DataType contains other DataType fields. __definition__ is list of (Name, DataTypeNode) tuples
            for definition in n.__definition__:
                candidateNodes.append(definition[1].id)

        if type(n) == nodes.VariableNode or type(n) == nodes.VariableTypeNode:
            if n.dataType is not None:
                candidateNodes.append(n.dataType)

        for ref in n.references:
            if not ref.source == n.id: raise Exception("Reference " + str(ref) + " has an invalid source")
            candidateNodes.append(ref.referenceType)
            candidateNodes.append(ref.target)

        # Uniquify candidateNodes and exclude nodes already present in nodeList
        candidateNodes = list(set(candidateNodes) - set(nodeList))

        # Add remaining candidateNodes to nodeList
        nodeList.extend(candidateNodes)

        # Inquire candidateNodes recursively if there are candidates left
        if len(candidateNodes) > 0:
            walkNodes(nodeSet, candidateNodes, nodeList)

    return nodeList

# Function for printing a set of nodeIds as Xml by filtering the referenceXml
# file. If existingXml is given, the generated Xml is merged with it
def printXML(nodeIds, referenceXmls, existingXml=None):
    # Start with empty nodeset XML
    # For now (if not merging), this will output uncomplete/invalid XML
    xmlRoot = etree.fromstring('''<UANodeSet
        xmlns:xsd="http://www.w3.org/2001/XMLSchema"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
            <Models/>
            <Aliases/>
        </UANodeSet>''')

    for referenceXml in referenceXmls:
        # Iterate through all reference XMLs looking fore nodes...
        referenceRoot = etree.parse(referenceXml.name).getroot()
        logger.info("Pulling in required nodes from {}...".format(referenceXml.name))

        # Collect models
        modelNodes = xmlRoot.find('Models', xmlRoot.nsmap)
        refModelNodes = referenceRoot.find('Models', referenceRoot.nsmap)
        for refModelNode in refModelNodes:
            if (refModelNode.attrib['ModelUri'] not in [model.attrib['ModelUri'] for model in modelNodes]):
                # Copy over model node if not already in xmlRoot
                modelNodes.append(refModelNode)
        referenceRoot.remove(refModelNodes)

        # Collect aliases
        aliasNodes = xmlRoot.find('Aliases', xmlRoot.nsmap)
        refAliasNodes = referenceRoot.find('Aliases', referenceRoot.nsmap)
        for refAliasNode in refAliasNodes:
            if (refAliasNode.attrib['Alias'] not in [alias.attrib['Alias'] for alias in aliasNodes]):
                # Copy over alias node if not already in xmlRoot
                aliasNodes.append(refAliasNode)
        referenceRoot.remove(refAliasNodes)

        # Look for other nodes we need
        for node in referenceRoot:
            if ('NodeId' in node.attrib) and NodeId(node.attrib['NodeId']) in nodeIds:
                # If node is in node list, copy over and remove from node list
                nodeIds.remove(NodeId(node.attrib['NodeId']))
                xmlRoot.append(node)

    if existingXml is not None:
        # Merge with existing Xml
        logger.info("Merging with {}...".format(existingXml.name))
        existingRoot = etree.parse(existingXml.name).getroot()

        # Handle model nodes
        modelNodes = xmlRoot.find('Models', xmlRoot.nsmap)
        existingModelNodes = existingRoot.find('Models', existingRoot.nsmap)
        for modelNode in modelNodes:
            if (modelNode.attrib['ModelUri'] not in [model.attrib['ModelUri'] for model in existingModelNodes]):
                # Append this model node if not already in existing XML
                existingModelNodes.append(modelNode)
        xmlRoot.remove(modelNodes)

        # Handle Alias nodes
        aliasNodes = xmlRoot.find('Aliases', xmlRoot.nsmap)
        existingAliasNodes = existingRoot.find('Aliases', existingRoot.nsmap)
        for aliasNode in aliasNodes:
            if (aliasNode.attrib['Alias'] not in [alias.attrib['Alias'] for alias in existingAliasNodes]):
                # Append this alias node if not already in existing XML
                existingAliasNodes.append(aliasNode)
        xmlRoot.remove(aliasNodes)

        # Handle all other nodes
        for node in xmlRoot:
            existingRoot.append(node)

        print(etree.tostring(existingRoot).decode('utf-8'))
    else:
        print(etree.tostring(xmlRoot).decode('utf-8'))

# Function for printg nodeIds (one nodeId per line)
def printNodeIds(nodeIds, namespaceMap=None):
    if namespaceMap is not None:
        # ExpandedNodeId
        nodeIdStrings = ['nsu={};i={}'.format(namespaceMap[node.ns], node.i) for node in nodeIds]
    else:
        # NodeId
        nodeIdStrings = ['ns={};i={}'.format(node.ns, node.i) for node in nodeIds]

    print(os.linesep.join(nodeIdStrings))

if nsCount == 0:
    logger.error("No files have been loaded")
    sys.exit(1)

logger.info("Collecting missing nodes...")
usedNodes = walkNodes(ns, ns.nodes)
missingNodes = [node for node in usedNodes if node not in ns.nodes]
logger.info("Collected {} missing nodes out of {} used nodes".format(len(missingNodes), len(usedNodes)))

# Load reference nodeset if given on command line
if len(args.refs) > 0:
    referenceNodeSet = NodeSet()

    # Copy namespace mapping from original nodeset to allow direct node
    # comparison between the two nodesets
    referenceNodeSet.namespaces = ns.namespaces

    for xmlfile in args.refs:
        if xmlfile.name in loadedFiles:
            logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
            continue
        loadedFiles.append(xmlfile.name)
        logger.info("Preprocessing (reference) " + str(xmlfile.name))
        referenceNodeSet.addNodeSet(xmlfile, True, typesArray="UA_TYPES")

    logger.info("Resolving all dependencies from references...")

    # Walk entire tree to find all dependencies
    dependentNodes = walkNodes(referenceNodeSet, missingNodes)

    # Remove dependencies already satisfied in existing NodeSet
    requiredNodes = [node for node in dependentNodes if node not in ns.nodes]
    unresolvedNodes = [node for node in requiredNodes if node not in referenceNodeSet.nodes]

    logger.info("Resolved {} required nodes out of {} dependent nodes ({} unresolved)"
        .format(len(requiredNodes), len(dependentNodes), len(unresolvedNodes)))

    if args.pull:
        # Run printXML when pull is specified. This will gather the requiredNodes
        # from the reference file and output it as XML or merge it into the
        # (first) existing XML file (if merge is specified)
        printXML(requiredNodes, args.refs, args.existing[0] if args.merge and len(args.existing) > 0 else None)
    else:
        printNodeIds(requiredNodes, referenceNodeSet.namespaces if args.expanded else None)
else:
    printNodeIds(missingNodes, ns.namespaces if args.expanded else None)
