#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)

from __future__ import print_function
import sys
import xml.dom.minidom as dom
import codecs
import re
import base64
import argparse

###############################
# Parse the Command Line Input#
###############################

parser = argparse.ArgumentParser()
parser.add_argument('-x', '--xml',
                    metavar="<nodeSetXML>",
                    dest="xmlfile",
                    help='NodeSet XML file with nodes that shall be generated.')

parser.add_argument('outputFile',
                    metavar='<outputFile>',
                    help='output file w/o extension')

args = parser.parse_args()

# Extract the BSD Blob from the XML file.
nodeset_base = open(args.xmlfile, "rb")
fileContent = nodeset_base.read()
# Remove BOM since the dom parser cannot handle it on python 3 windows
if fileContent.startswith(codecs.BOM_UTF8):
    fileContent = fileContent.lstrip(codecs.BOM_UTF8)
if (sys.version_info >= (3, 0)):
    fileContent = fileContent.decode("utf-8")

# Remove the uax namespace from tags. UaModeler adds this namespace to some elements
fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\\g<1>\\g<2>\\g<3>>", fileContent)

nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
if len(nodesets) == 0 or len(nodesets) > 1:
    raise Exception("contains no or more then 1 nodeset")
nodeset = nodesets[0]
variableNodes = nodeset.getElementsByTagName("UAVariable")
for nd in variableNodes:
    if (nd.hasAttribute("SymbolicName") and (re.match(r".*_BinarySchema", nd.attributes["SymbolicName"].nodeValue) or nd.attributes["SymbolicName"].nodeValue == "TypeDictionary_BinarySchema")) or (nd.hasAttribute("ParentNodeId") and not nd.hasAttribute("SymbolicName") and re.fullmatch(r"i=93", nd.attributes["ParentNodeId"].nodeValue)):
        type_content = nd.getElementsByTagName("Value")[0].getElementsByTagName("ByteString")[0]
        f = open(args.outputFile, 'w')
        f.write(base64.b64decode(type_content.firstChild.nodeValue).decode("utf-8"))
        f.flush()
        f.close()
