#!/usr/bin/env python
# -*- coding: utf-8 -*-

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
### file for more information on the MPLv2 terms and restrictions).
###
### This program is not meant to be used in a production environment. The
### author is not liable for any complications arising due to the use of
### this program.
###

import logging
import argparse
from nodeset import *
from backend_open62541 import generateOpen62541Code

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

parser.add_argument('outputFile',
                    metavar='<outputFile>',
                    help='The path/basename for the <output file>.c and <output file>.h files to be generated. This will also be the function name used in the header and c-file.')

parser.add_argument('--generate-ns0',
                    action='store_true',
                    dest="generate_ns0",
                    help='Omit some consistency checks for bootstrapping namespace 0, create references to parents and type definitions manually')

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

parser.add_argument('--max-string-length',
                    type=int,
                    dest="max_string_length",
                    default=0,
                    help='Maximum allowed length of a string literal. If longer, it will be set to an empty string')

parser.add_argument('--encode-binary-size',
                    type=int,
                    dest="encode_binary_size",
                    default=32000,
                    help='Size of the temporary array used to encode custom datatypes. If you don\'t know what it is, do not use this option')

parser.add_argument('-v', '--verbose', action='count',
                    default=1,
                    help='Make the script more verbose. Can be applied up to 4 times')

args = parser.parse_args()

# Set up logging
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
verbosity = 0
if args.verbose:
    verbosity = int(args.verbose)
if (verbosity == 1):
    logging.basicConfig(level=logging.ERROR)
elif (verbosity == 2):
    logging.basicConfig(level=logging.WARNING)
elif (verbosity == 3):
    logging.basicConfig(level=logging.INFO)
elif (verbosity >= 4):
    logging.basicConfig(level=logging.DEBUG)
else:
    logging.basicConfig(level=logging.CRITICAL)

# Create a new nodeset. The nodeset name is not significant.
# Parse the XML files
ns = NodeSet()
nsCount = 0

def getTypesArray(nsIdx):
    if nsIdx < len(args.typesArray):
        return args.typesArray[nsIdx]
    else:
        return "UA_TYPES"

for xmlfile in args.existing:
    logger.info("Preprocessing (existing) " + str(xmlfile.name))
    ns.addNodeSet(xmlfile, True, typesArray=getTypesArray(nsCount))
    nsCount +=1
for xmlfile in args.infiles:
    logger.info("Preprocessing " + str(xmlfile.name))
    ns.addNodeSet(xmlfile, typesArray=getTypesArray(nsCount))
    nsCount +=1

# # We need to notify the open62541 server of the namespaces used to be able to use i.e. ns=3
# namespaceArrayNames = preProc.getUsedNamespaceArrayNames()
# for key in namespaceArrayNames:
#   ns.addNamespace(key, namespaceArrayNames[key])

# Remove blacklisted nodes from the nodeset
# Doing this now ensures that unlinkable pointers will be cleanly removed
# during sanitation.
for blacklist in args.blacklistFiles:
    for line in blacklist.readlines():
        line = line.replace(" ", "")
        id = line.replace("\n", "")
        if ns.getNodeByIDString(id) == None:
            logger.info("Can't blacklist node, namespace does currently not contain a node with id " + str(id))
        else:
            ns.removeNodeById(line)
    blacklist.close()

# Set the nodes from the ignore list to hidden. This removes them from dependency calculation
# and from printing their generated code.
# These nodes should be already pre-created on the server to avoid any errors during
# creation.
for ignoreFile in args.ignoreFiles:
    for line in ignoreFile.readlines():
        line = line.replace(" ", "")
        id = line.replace("\n", "")
        ns.hide_node(NodeId(id))
        #if not ns.hide_node(NodeId(id)):
        #    logger.info("Can't ignore node, namespace does currently not contain a node with id " + str(id))
    ignoreFile.close()

# Remove nodes that are not printable or contain parsing errors, such as
# unresolvable or no references or invalid NodeIDs
ns.sanitize()


# Parse Datatypes in order to find out what the XML keyed values actually
# represent.
# Ex. <rpm>123</rpm> is not encodable
#     only after parsing the datatypes, it is known that
#     rpm is encoded as a double
ns.buildEncodingRules()

# Allocate/Parse the data values. In order to do this, we must have run
# buidEncodingRules.
ns.allocateVariables()

#printDependencyGraph(ns)

# Create the C code with the open62541 backend of the compiler
logger.info("Generating Code")
generateOpen62541Code(ns, args.outputFile, args.generate_ns0, args.internal_headers, args.typesArray, args.max_string_length, args.encode_binary_size)
logger.info("NodeSet generation code successfully printed")
