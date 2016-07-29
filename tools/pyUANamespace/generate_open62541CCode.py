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

from __future__ import print_function
from ua_namespace import *
import logging
import argparse
from open62541_XMLPreprocessor import open62541_XMLPreprocessor

logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser(
    description="""Parse OPC UA NamespaceXML file(s) and create C code for generating nodes in open62541

generate_open62541CCode.py will first read all XML files passed on the command line, then link and check the namespace. All nodes that fulfill the basic requirements will then be printed as C-Code intended to be included in the open62541 OPC UA Server that will initialize the corresponding namespace.""",
    formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('infiles',
                    metavar="<namespaceXML>",
                    nargs='+',
                    type=argparse.FileType('r'),
                    help='Namespace XML file(s). Note that the last definition of a node encountered will be used and all prior definitions are discarded.')
parser.add_argument('outputFile',
                    metavar='<outputFile>',
                    #type=argparse.FileType('w', 0),
                    help='The basename for the <output file>.c and <output file>.h files to be generated. This will also be the function name used in the header and c-file.')
parser.add_argument('-i','--ignore',
                    metavar="<ignoreFile>",
                    type=argparse.FileType('r'),
                    action='append',
                    dest="ignoreFiles",
                    default=[],
                    help='Loads a list of NodeIDs stored in ignoreFile (one NodeID per line). The compiler will assume that these nodes have been created externally and not generate any code for them. They will however be linked to from other nodes.')
parser.add_argument('-b','--blacklist',
                    metavar="<blacklistFile>",
                    type=argparse.FileType('r'),
                    action='append',
                    dest="blacklistFiles",
                    default=[],
                    help='Loads a list of NodeIDs stored in blacklistFile (one NodeID per line). Any of the nodeIds encountered in this file will be removed from the namespace prior to compilation. Any references to these nodes will also be removed')
parser.add_argument('-s','--suppress',
                    metavar="<attribute>",
                    action='append',
                    dest="suppressedAttributes",
                    choices=['description', 'browseName', 'displayName', 'writeMask', 'userWriteMask','nodeid'],
                    default=[],
                    help="Suppresses the generation of some node attributes. Currently supported options are 'description', 'browseName', 'displayName', 'writeMask', 'userWriteMask' and 'nodeid'.")

parser.add_argument('-v','--verbose', action='count', help='Make the script more verbose. Can be applied up to 4 times')

args = parser.parse_args()

level = logging.CRITICAL
verbosity = 0
if args.verbose:
  verbosity = int(args.verbose)
if (verbosity==1):
  level = logging.ERROR
elif (verbosity==2):
  level = logging.WARNING
elif (verbosity==3):
  level = logging.INFO
elif (verbosity>=4):
  level = logging.DEBUG

logging.basicConfig(level=level)
logger.setLevel(logging.INFO)

# Creating the header is tendious. We can skip the entire process if
# the header exists.
#if path.exists(argv[-1]+".c") or path.exists(argv[-1]+".h"):
#  log(None, "File " + str(argv[-1]) + " does already exists.", LOG_LEVEL_INFO)
#  log(None, "Header generation will be skipped. Delete the header and rerun this script if necessary.", LOG_LEVEL_INFO)
#  exit(0)

# Open the output file
outfileh = open(args.outputFile+".h", r"w+")
outfilec = open(args.outputFile+".c", r"w+")

# Create a new namespace. Note that the namespace name is not significant.
ns = opcua_namespace("open62541")

# Clean up the XML files by removing duplicate namespaces and unwanted prefixes
preProc = open62541_XMLPreprocessor()
for xmlfile in args.infiles:
  logger.info("Preprocessing " + str(xmlfile.name))
  preProc.addDocument(xmlfile.name)
preProc.preprocessAll()

# Parse the XML files
for xmlfile in preProc.getPreProcessedFiles():
  logger.info("Parsing " + str(xmlfile))
  ns.parseXML(xmlfile)

# We need to notify the open62541 server of the namespaces used to be able to use i.e. ns=3
namespaceArrayNames = preProc.getUsedNamespaceArrayNames()
for key in namespaceArrayNames:
  ns.addNamespace(key, namespaceArrayNames[key])

# Remove any temp files - they are not needed after the AST is created
preProc.removePreprocessedFiles()

# Remove blacklisted nodes from the namespace
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

# Link the references in the namespace
logger.info("Linking namespace nodes and references")
ns.linkOpenPointers()

# Remove nodes that are not printable or contain parsing errors, such as
# unresolvable or no references or invalid NodeIDs
ns.sanitize()

# Parse Datatypes in order to find out what the XML keyed values actually
# represent.
# Ex. <rpm>123</rpm> is not encodable
#     only after parsing the datatypes, it is known that
#     rpm is encoded as a double
logger.info("Building datatype encoding rules")
ns.buildEncodingRules()

# Allocate/Parse the data values. In order to do this, we must have run
# buidEncodingRules.
logger.info("Allocating variables")
ns.allocateVariables()

# Users may have manually defined some nodes in their code already (such as serverStatus).
# To prevent those nodes from being reprinted, we will simply mark them as already
# converted to C-Code. That way, they will still be referred to by other nodes, but
# they will not be created themselves.
ignoreNodes = []
for ignore in args.ignoreFiles:
  for line in ignore.readlines():
    line = line.replace(" ","")
    id = line.replace("\n","")
    if ns.getNodeByIDString(id) == None:
      logger.warn("Can't ignore node, Namespace does currently not contain a node with id " + str(id))
    else:
      ignoreNodes.append(ns.getNodeByIDString(id))
  ignore.close()

# Create the C Code
logger.info("Generating Header")
# Returns a tuple of (["Header","lines"],["Code","lines","generated"])
from os.path import basename
generatedCode = ns.printOpen62541Header(ignoreNodes, args.suppressedAttributes, outfilename=basename(args.outputFile))
for line in generatedCode[0]:
  outfileh.write(line+"\n")
for line in generatedCode[1]:
  outfilec.write(line+"\n")

outfilec.close()
outfileh.close()

logger.info("Namespace generation code successfully printed")
