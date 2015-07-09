#!/usr/bin/python
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

from sys import argv, exit
from os import path
from ua_namespace import *
from logger import *

def usage():
  print "Script usage:"
  print "generate_open62541CCode [-i <ignorefile> | -b <blacklistfile>] <namespace XML> [namespace.xml[ namespace.xml[...]]] <output file>"
  print ""
  print "generate_open62541CCode will first read all XML files passed on the command line, then "
  print "link and check the namespace. All nodes that fullfill the basic requirements will then be"
  print "printed as C-Code intended to be included in the open62541 OPC-UA Server that will"
  print "initialize the corresponding name space."
  print ""
  print "Manditory Arguments:"
  print "<namespace XML>    At least one Namespace XML file must be passed."
  print "<output file>      The basename for the <output file>.c and <output file>.h files to be generated."
  print "                   This will also be the function name used in the header and c-file."
  print ""
  print ""
  print "Additional Arguments:"
  print """   -i <ignoreFile>     Loads a list of NodeIDs stored in ignoreFile (one NodeID per line)
                       The compiler will assume that these Nodes have been created externally
                       and not generate any code for them. They will however be linked to
                       from other nodes."""
  print """   -b <blacklistFile>  Loads a list of NodeIDs stored in blacklistFile (one NodeID per line)
                       Any of the nodeIds encountered in this file will be removed from the namespace
                       prior to compilation. Any references to these nodes will also be removed"""
  print """   -s <attribute>  Suppresses the generation of some node attributes. Currently supported
                       options are 'description', 'browseName', 'displayName', 'writeMask', 'userWriteMask'
                       and 'nodeid'."""
  print """   namespaceXML Any number of namespace descriptions in XML format. Note that the
                       last description of a node encountered will be used and all prior definitions
                       are discarded."""

if __name__ == '__main__':
  # Check if the parameters given correspond to actual files
  infiles = []
  ouffile = ""
  ignoreFiles = []
  blacklistFiles = []
  supressGenerationOfAttribute=[]

  GLOBAL_LOG_LEVEL = LOG_LEVEL_DEBUG
  
  arg_isIgnore    = False
  arg_isBlacklist = False
  arg_isSupress   = False
  if len(argv) < 2:
    usage()
    exit(1)
  for filename in argv[1:-1]:
    if arg_isIgnore:
      arg_isIgnore = False
      if path.exists(filename):
        ignoreFiles.append(filename)
      else:
        log(None, "File " + str(filename) + " does not exist.", LOG_LEVEL_ERROR)
        usage()
        exit(1)
    elif arg_isBlacklist:
      arg_isBlacklist = False
      if path.exists(filename):
        blacklistFiles.append(filename)
      else:
        log(None, "File " + str(filename) + " does not exist.", LOG_LEVEL_ERROR)
        usage()
        exit(1)
    elif arg_isSupress:
      arg_isSupress = False
      supressGenerationOfAttribute.append(filename.lower())
    else:
      if path.exists(filename):
        infiles.append(filename)
      elif filename.lower() == "-i" or filename.lower() == "--ignore" :
        arg_isIgnore = True
      elif filename.lower() == "-b" or filename.lower() == "--blacklist" :
        arg_isBlacklist = True
      elif filename.lower() == "-s" or filename.lower() == "--suppress" :
        arg_isSupress = True
      else:
        log(None, "File " + str(filename) + " does not exist.", LOG_LEVEL_ERROR)
        usage()
        exit(1)

  # Creating the header is tendious. We can skip the entire process if
  # the header exists.
  #if path.exists(argv[-1]+".c") or path.exists(argv[-1]+".h"):
  #  log(None, "File " + str(argv[-1]) + " does already exists.", LOG_LEVEL_INFO)
  #  log(None, "Header generation will be skipped. Delete the header and rerun this script if necessary.", LOG_LEVEL_INFO)
  #  exit(0)

  # Open the output file
  outfileh = open(argv[-1]+".h", r"w+")
  outfilec = open(argv[-1]+".c", r"w+")

  # Create a new namespace
  # Note that the name is actually completely symbolic, it has no other
  # function but to distinguish this specific class.
  # A namespace class acts as a container for nodes. The nodes may belong
  # to any number of different OPC-UA namespaces.
  ns = opcua_namespace("open62541")

  # Parse the XML files
  for xmlfile in infiles:
    log(None, "Parsing " + str(xmlfile), LOG_LEVEL_INFO)
    ns.parseXML(xmlfile)

  # Remove blacklisted nodes from the namespace
  # Doing this now ensures that unlinkable pointers will be cleanly removed
  # during sanitation.
  for blacklist in blacklistFiles:
    bl = open(blacklist, "r")
    for line in bl.readlines():
      line = line.replace(" ","")
      id = line.replace("\n","")
      if ns.getNodeByIDString(id) == None:
        log(None, "Can't blacklist node, namespace does currently not contain a node with id " + str(id), LOG_LEVEL_WARN)
      else:
        ns.removeNodeById(line)
    bl.close()

  # Link the references in the namespace
  log(None, "Linking namespace nodes and references", LOG_LEVEL_INFO)
  ns.linkOpenPointers()

  # Remove nodes that are not printable or contain parsing errors, such as
  # unresolvable or no references or invalid NodeIDs
  ns.sanitize()

  # Parse Datatypes in order to find out what the XML keyed values actually
  # represent.
  # Ex. <rpm>123</rpm> is not encodable
  #     only after parsing the datatypes, it is known that
  #     rpm is encoded as a double
  log(None, "Building datatype encoding rules", LOG_LEVEL_INFO)
  ns.buildEncodingRules()

  # Allocate/Parse the data values. In order to do this, we must have run
  # buidEncodingRules.
  log(None, "Allocating variables", LOG_LEVEL_INFO)
  ns.allocateVariables()

  # Users may have manually defined some nodes in their code already (such as serverStatus).
  # To prevent those nodes from being reprinted, we will simply mark them as already
  # converted to C-Code. That way, they will still be reffered to by other nodes, but
  # they will not be created themselves.
  ignoreNodes = []
  for ignore in ignoreFiles:
    ig = open(ignore, "r")
    for line in ig.readlines():
      line = line.replace(" ","")
      id = line.replace("\n","")
      if ns.getNodeByIDString(id) == None:
        log(None, "Can't ignore node, Namespace does currently not contain a node with id " + str(id), LOG_LEVEL_WARN)
      else:
        ignoreNodes.append(ns.getNodeByIDString(id))
    ig.close()
  
  # Create the C Code
  log(None, "Generating Header", LOG_LEVEL_INFO)
  # Returns a tuple of (["Header","lines"],["Code","lines","generated"])
  generatedCode=ns.printOpen62541Header(ignoreNodes, supressGenerationOfAttribute, outfilename=path.basename(argv[-1]))
  for line in generatedCode[0]:
    outfileh.write(line+"\n")
  for line in generatedCode[1]:
    outfilec.write(line+"\n")
 
  outfilec.close()
  outfileh.close()

  exit(0)
