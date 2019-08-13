#!/usr/bin/env python
# -*- coding: utf-8 -*-

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

"""
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Authors:
- Chris Iatrou (ichrispa@core-vector.net)
- Julius Pfrommer
- Stefan Profanter (profanter@fortiss.org)
"""

import logging
import argparse
from datatypes import NodeId
from nodeset import *

if __name__ == '__main__':
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
                        help='The path/basename for the <output file>.c and <output file>.h files to be generated. '
                             'This will also be the function name used in the header and c-file.')

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
                        help='Loads a list of NodeIDs stored in blacklistFile (one NodeID per line). '
                             'Any of the nodeIds encountered in this file will be removed from the nodeset prior to '
                             'compilation. Any references to these nodes will also be removed')

    parser.add_argument('-i', '--ignore',
                        metavar="<ignoreFile>",
                        type=argparse.FileType('r'),
                        action='append',
                        dest="ignoreFiles",
                        default=[],
                        help='Loads a list of NodeIDs stored in ignoreFile (one NodeID per line). '
                             'Any of the nodeIds encountered in this file will be kept in the nodestore but not '
                             'printed in the generated code')

    parser.add_argument('-t', '--types-array',
                        metavar="<typesArray>",
                        action='append',
                        type=str,
                        dest="typesArray",
                        default=[],
                        help='Types array for the given namespace. Can be used mutliple times to define '
                             '(in the same order as the .xml files, first for --existing, then --xml) the type arrays')

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
    nodeset = NodeSet()
    nsCount = 0
    loadedFiles = list()


    def get_types_array(namespace_index):
        if namespace_index < len(args.typesArray):
            return args.typesArray[namespace_index]
        else:
            return "UA_TYPES"


    for xml_file in args.existing:
        if xml_file.name in loadedFiles:
            logger.info("Skipping Nodeset since it is already loaded: {} ".format(xml_file.name))
            continue
        loadedFiles.append(xml_file.name)
        logger.info("Preprocessing (existing) " + str(xml_file.name))
        nodeset.add_node_set(xml_file, True, types_array=get_types_array(nsCount))
        nsCount += 1
    for xml_file in args.infiles:
        if xml_file.name in loadedFiles:
            logger.info("Skipping Nodeset since it is already loaded: {} ".format(xml_file.name))
            continue
        loadedFiles.append(xml_file.name)
        logger.info("Preprocessing " + str(xml_file.name))
        nodeset.add_node_set(xml_file, types_array=get_types_array(nsCount))
        nsCount += 1

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
            node_id = line.replace("\n", "")
            if nodeset.getNodeByIDString(node_id) is None:
                logger.info("Can't blacklist node, namespace does currently not contain a node with id " + str(node_id))
            else:
                nodeset.removeNodeById(line)
        blacklist.close()

    # Set the nodes from the ignore list to hidden. This removes them from dependency calculation
    # and from printing their generated code.
    # These nodes should be already pre-created on the server to avoid any errors during
    # creation.
    for ignoreFile in args.ignoreFiles:
        for line in ignoreFile.readlines():
            line = line.replace(" ", "")
            node_id = line.replace("\n", "")
            nodeset.hide_node(NodeId(node_id))
            # if not ns.hide_node(NodeId(id)):
            #     logger.info("Can't ignore node, namespace does currently not contain a node with id " + str(id))
        ignoreFile.close()

    # Remove nodes that are not printable or contain parsing errors, such as
    # unresolvable or no references or invalid NodeIDs
    nodeset.sanitize()

    # Parse Datatypes in order to find out what the XML keyed values actually
    # represent.
    # Ex. <rpm>123</rpm> is not encodable
    #     only after parsing the datatypes, it is known that
    #     rpm is encoded as a double
    nodeset.build_encoding_rules()

    # Allocate/Parse the data values. In order to do this, we must have run
    # buidEncodingRules.
    nodeset.allocate_variables()

    nodeset.add_inverse_references()

    nodeset.set_node_parent()

    logger.info("Generating Code for Backend: {}".format(args.backend))

    if args.backend == "open62541":
        # Create the C code with the open62541 backend of the compiler
        from backend_open62541 import generate_open62541_code

        generate_open62541_code(nodeset, args.outputFile, args.internal_headers, args.typesArray)
    elif args.backend == "graphviz":
        from backend_graphviz import generateGraphvizCode

        generateGraphvizCode(nodeset, filename=args.outputFile)
    else:
        logger.error("Unsupported backend: {}".format(args.backend))
        exit(1)

    logger.info("NodeSet generation code successfully printed")
