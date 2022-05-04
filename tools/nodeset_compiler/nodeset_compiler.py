#!/usr/bin/env python3
# -*- coding: utf-8 -*-

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH


import logging
import argparse
import sys
from datatypes import NodeId
from nodeset import *

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
                    help='NodeSet XML files with nodes that shall be generated.')

parser.add_argument('--bsd',
                    metavar="<nodeSetBSD>",
                    type=argparse.FileType('rb'),
                    action='append',
                    dest="bsdFile",
                    default=[],
                    help='Nodeset BSD file which must be specified if self-defined data types are contained in the XML and the BSD blob is not integrated in the XML.')

parser.add_argument('outputFile',
                    metavar='<outputFile>',
                    help='The path/basename for the <output file>.c and <output file>.h files to be generated. This will also be the function name used in the header and c-file.')

parser.add_argument('--internal-headers',
                    action='store_true',
                    dest="internal_headers",
                    help='Include internal headers instead of amalgamated header')

parser.add_argument('-b', '--blacklist',
                    metavar="<blacklistFile>",
                    type=argparse.FileType('r'),
                    action='append',
                    dest="blacklistFiles",
                    default=[],
                    help='Loads a list of NodeIDs stored in blacklistFile (one NodeID per line). Any of the nodeIds encountered in this file will be removed from the nodeset prior to compilation. Any references to these nodes will also be removed')

parser.add_argument('-i', '--ignore',
                    metavar="<ignoreFile>",
                    type=argparse.FileType('r'),
                    action='append',
                    dest="ignoreFiles",
                    default=[],
                    help='Loads a list of NodeIDs stored in ignoreFile (one NodeID per line). Any of the nodeIds encountered in this file will be kept in the nodestore but not printed in the generated code')

parser.add_argument('-t', '--types-array',
                    metavar="<typesArray>",
                    action='append',
                    type=str,
                    dest="typesArray",
                    default=[],
                    help='Types array for the given namespace. Can be used mutliple times to define (in the same order as the .xml files, first for --existing, then --xml) the type arrays')

parser.add_argument('-v', '--verbose', action='count',
                    default=1,
                    help='Make the script more verbose. Can be applied up to 4 times')

parser.add_argument('--backend',
                    default='open62541',
                    const='open62541',
                    nargs='?',
                    choices=['open62541', 'graphviz'],
                    help='Backend for the output files (default: %(default)s)')

args = parser.parse_args()

# Set up logging
# By default logging outputs to stderr. We want to redirect it to stdout, otherwise build output from cmake
# is in stdout and nodeset compiler in stderr
logging.basicConfig(stream=sys.stdout)
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

# Remove duplicate entries in typesArray, which occur when a nodeset has more than one depend.
tmp_list = []
for e in args.typesArray:
    if e not in tmp_list:
        tmp_list.append(e)
args.typesArray = tmp_list
def getTypesArray(nsIdx):
    if nsIdx < len(args.typesArray):
        return args.typesArray[nsIdx]
    else:
        return "UA_TYPES"

for xmlfile in args.existing:
    if xmlfile.name in loadedFiles:
        logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
        continue
    loadedFiles.append(xmlfile.name)
    logger.info("Preprocessing (existing) " + str(xmlfile.name))
    ns.addNodeSet(xmlfile, True, typesArray=getTypesArray(nsCount))
    nsCount +=1
for xmlfile in args.infiles:
    if xmlfile.name in loadedFiles:
        logger.info("Skipping Nodeset since it is already loaded: {} ".format(xmlfile.name))
        continue
    loadedFiles.append(xmlfile.name)
    logger.info("Preprocessing " + str(xmlfile.name))
    ns.addNodeSet(xmlfile, typesArray=getTypesArray(nsCount))
    nsCount +=1

# # We need to notify the open62541 server of the namespaces used to be able to use i.e. ns=3
# namespaceArrayNames = preProc.getUsedNamespaceArrayNames()
# for key in namespaceArrayNames:
#   ns.addNamespace(key, namespaceArrayNames[key])

# Set the nodes from the ignore list to hidden. This removes them from
# dependency calculation and from printing their generated code. These nodes
# should be already pre-created on the server to avoid any errors during
# creation.
for ignoreFile in args.ignoreFiles:
    for line in ignoreFile.readlines():
        line = line.replace(" ", "")
        id = line.replace("\n", "")
        ns.hide_node(NodeId(id))
        #if not ns.hide_node(NodeId(id)):
        #    logger.info("Cannot ignore node, namespace does currently not contain a node with id " + str(id))
    ignoreFile.close()

# Remove nodes that are not printable or contain parsing errors, such as
# unresolvable or no references or invalid NodeIDs
ns.sanitize()

# Generate the BSD file from the XML.
ns.generateParser(args.existing, args.infiles, args.bsdFile)

# Allocate/Parse the data values. In order to do this, we must have run
# buidEncodingRules.
ns.allocateVariables()

# Create inverse references
ns.addInverseReferences()

# Remove blacklisted nodes from the nodeset. We need to have the inverse
# references here to ensure the reference is deleted from the referencing node
# too
if args.blacklistFiles:
    for blacklist in args.blacklistFiles:
        for line in blacklist.readlines():
            if line.startswith("#"):
                continue
            line = line.replace(" ", "")
            id = line.replace("\n", "")
            if len(id) == 0:
                continue
            n = ns.getNodeByIDString(id)
            if n is None:
                logger.debug("Cannot blacklist node, namespace does currently not contain a node with id " + str(id))
            else:
                ns.remove_node(n)
        blacklist.close()
    ns.sanitize()

# Figure out from the references which is the parent for each node
ns.setNodeParent()

logger.info("Generating Code for Backend: {}".format(args.backend))

if args.backend == "open62541":
    # Create the C code with the open62541 backend of the compiler
    from backend_open62541 import generateOpen62541Code
    generateOpen62541Code(ns, args.outputFile, args.internal_headers, args.typesArray)
elif args.backend == "graphviz":
    from backend_graphviz import generateGraphvizCode
    generateGraphvizCode(ns, filename=args.outputFile)
else:
    logger.error("Unsupported backend: {}".format(args.backend))
    exit(1)

logger.info("NodeSet generation code successfully printed")
