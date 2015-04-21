#!/usr/bin/env/python
# -*- coding: utf-8 -*-

###
### Author:  Chris Iatrou (ichrispa@core-vector.net)
### Version: rev 13
###
### This program was created for educational purposes and is released into the
### public domain under the General Public Licence. A copy of the GNU GPL is
### available under http://www.gnu.org/licenses/gpl-3.0.html.
###
### This program is not meant to be used in a production environment. The
### author is not liable for any complications arising due to the use of
### this program.
###

from sys import argv, exit
from os import path
from ua_namespace import *

def usage():
  print "Skript usage:"
  print "generate_open62541CCode <namespace XML> [namespace.xml[ namespace.xml[...]]] <output file>"
  print ""
  print "generate_open62541CCode will first read all XML files passed on the command line, then "
  print "link and check the namespace. All nodes that fullfill the basic requirements will then be"
  print "printed as C-Code intended to be included in the open62541 OPC-UA Server that will"
  print "initialize the corresponding name space."

if __name__ == '__main__':
  # Check if the parameters given correspond to actual files
  infiles = []
  ouffile = ""
  if len(argv) < 2:
    usage()
    exit(1)
  for filename in argv[1:-1]:
    if path.exists(filename):
      infiles.append(filename)
    else:
      print "File " + str(filename) + " does not exist."
      usage()
      exit(1)

  # Creating the header is tendious. We can skip the entire process if
  # the header exists.
#  if path.exists(argv[-1]):
#    print "Header " + str(argv[-1]) + " already exists."
#    print "Header generation is a lengthy process, please delete the header"
#    print "  if you really want namespace 0 to be recompiled from the XML"
#    print "  file."
#    exit(0)

  # Open the output file
  outfile = open(argv[-1], r"w+")

  # Create a new namespace
  # Note that the name is actually completely symbolic, it has no other
  # function but to distinguish this specific class.
  # A namespace class acts as a container for nodes. The nodes may belong
  # to any number of different OPC-UA namespaces.
  ns = opcua_namespace("open62541")

  # Parse the XML files
  for xmlfile in infiles:
    print "Parsing " + xmlfile
    ns.parseXML(xmlfile)

  # Link the references in the namespace
  ns.linkOpenPointers()

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

  # Create the C Code
  for line in ns.printOpen62541Header():
    outfile.write(line+"\n")

  outfile.close()