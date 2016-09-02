#!/usr/bin/python
# -*- coding: utf-8 -*-

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
### Version: rev 14
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

import logging
import argparse
from nodeset import *
from backend_open62541 import generateOpen62541Code

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('-e','--existing',
                    metavar="<existingNodeSetXML>",
                    type=argparse.FileType('r'),
                    dest="existing",
                    action='append',
                    default=[],
                    help='NodeSet XML files with nodes that are already present on the server.')

parser.add_argument('infiles',
                    metavar="<nodeSetXML>",
                    action='append',
                    type=argparse.FileType('r'),
                    default=[],
                    help='NodeSet XML files with nodes that shall be generated.')

parser.add_argument('outputFile',
                    metavar='<outputFile>',
                    help='The path/basename for the <output file>.c and <output file>.h files to be generated. This will also be the function name used in the header and c-file.')

parser.add_argument('--generate-ns0',
                    action='store_true',
                    dest="generate_ns0",
                    help='Omit some consistency checks for bootstrapping namespace 0, create references to parents and type definitions manually')

parser.add_argument('-b','--blacklist',
                    metavar="<blacklistFile>",
                    type=argparse.FileType('r'),
                    action='append',
                    dest="blacklistFiles",
                    default=[],
                    help='Loads a list of NodeIDs stored in blacklistFile (one NodeID per line). Any of the nodeIds encountered in this file will be removed from the nodeset prior to compilation. Any references to these nodes will also be removed')

parser.add_argument('-s','--suppress',
                    metavar="<attribute>",
                    action='append',
                    dest="suppressedAttributes",
                    choices=['description', 'browseName', 'displayName', 'writeMask', 'userWriteMask','nodeid'],
                    default=[],
                    help="Suppresses the generation of some node attributes. Currently supported options are 'description', 'browseName', 'displayName', 'writeMask', 'userWriteMask' and 'nodeid'.")

parser.add_argument('-v','--verbose', action='count', help='Make the script more verbose. Can be applied up to 4 times')

args = parser.parse_args()

# Set up logging
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
verbosity = 0
if args.verbose:
  verbosity = int(args.verbose)
if (verbosity==1):
  logging.basicConfig(level=logging.ERROR)
elif (verbosity==2):
  logging.basicConfig(level=logging.WARNING)
elif (verbosity==3):
  logging.basicConfig(level=logging.INFO)
elif (verbosity>=4):
  logging.basicConfig(level=logging.DEBUG)
else:
  logging.basicConfig(level=logging.CRITICAL)

# Create a new nodeset. The nodeset name is not significant.
# Parse the XML files
ns = NodeSet()
for xmlfile in args.existing:
  logger.info("Preprocessing (existing) " + str(xmlfile.name))
  ns.addNodeSet(xmlfile, True)
for xmlfile in args.infiles:
  logger.info("Preprocessing " + str(xmlfile.name))
  ns.addNodeSet(xmlfile)

# # We need to notify the open62541 server of the namespaces used to be able to use i.e. ns=3
# namespaceArrayNames = preProc.getUsedNamespaceArrayNames()
# for key in namespaceArrayNames:
#   ns.addNamespace(key, namespaceArrayNames[key])

# Remove blacklisted nodes from the nodeset
# Doing this now ensures that unlinkable pointers will be cleanly removed
# during sanitation.
for blacklist in args.blacklistFiles:
  for line in blacklist.readlines():
    line = line.replace(" ","")
    id = line.replace("\n","")
    if ns.getNodeByIDString(id) == None:
      logger.info("Can't blacklist node, namespace does currently not contain a node with id " + str(id))
    else:
      ns.removeNodeById(line)
  blacklist.close()

# Remove nodes that are not printable or contain parsing errors, such as
# unresolvable or no references or invalid NodeIDs
ns.sanitize()

# Create the C code with the open62541 backend of the compiler
logger.info("Generating Code")
generateOpen62541Code(ns, args.outputFile, args.suppressedAttributes, args.generate_ns0)
logger.info("NodeSet generation code successfully printed")
