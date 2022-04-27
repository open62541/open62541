#!/usr/bin/env python3
# -*- coding: utf-8 -*-

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH

from __future__ import print_function
import sys
import xml.dom.minidom as dom
import logging
import codecs
import re
from datatypes import NodeId, valueIsInternalType
from nodes import *
from opaque_type_mapping import opaque_type_mapping

from type_parser import CSVBSDTypeParser
import io
import tempfile
import base64

__all__ = ['NodeSet', 'getSubTypesOf']

logger = logging.getLogger(__name__)

if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s
    string_types = str
else:
    string_types = basestring

####################
# Helper Functions #
####################

hassubtype = NodeId("ns=0;i=45")

def getSubTypesOf(nodeset, node, skipNodes=[]):
    if node in skipNodes:
        return []
    re = set()
    re.add(node)
    for ref in node.references:
        if (ref.referenceType == hassubtype):
            skipAll = set()
            skipAll.update(skipNodes)
            skipAll.update(re)
            if (ref.source == node.id and ref.isForward):
                re.update(getSubTypesOf(nodeset, nodeset.nodes[ref.target], skipNodes=skipAll))
            elif (ref.target == node.id and not ref.isForward):
                re.update(getSubTypesOf(nodeset, nodeset.nodes[ref.source], skipNodes=skipAll))
    return re

def extractNamespaces(xmlfile):
    # Extract a list of namespaces used. The first namespace is always
    # "http://opcfoundation.org/UA/". minidom gobbles up
    # <NamespaceUris></NamespaceUris> elements, without a decent way to reliably
    # access this dom2 <uri></uri> elements (only attribute xmlns= are accessible
    # using minidom). We need them for dereferencing though... This function
    # attempts to do just that.

    namespaces = ["http://opcfoundation.org/UA/"]
    infile = codecs.open(xmlfile.name, encoding='utf-8')
    foundURIs = False
    nsline = ""
    for line in infile:
        if "<namespaceuris>" in line.lower():
            foundURIs = True
        elif "</namespaceuris>" in line.lower():
            nsline = nsline + line
            break
        if foundURIs:
            nsline = nsline + line

    if len(nsline) > 0:
        ns = dom.parseString(nsline).getElementsByTagName("NamespaceUris")
        for uri in ns[0].childNodes:
            if uri.nodeType != uri.ELEMENT_NODE:
                continue
            if uri.firstChild.data in namespaces:
                continue
            namespaces.append(uri.firstChild.data)
    infile.close()
    return namespaces

def buildAliasList(xmlelement):
    """Parses the <Alias> XML Element present in must XML NodeSet definitions.
       Contents the Alias element are stored in a dictionary for further
       dereferencing during pointer linkage (see linkOpenPointer())."""
    aliases = {}
    for al in xmlelement.childNodes:
        if al.nodeType == al.ELEMENT_NODE:
            if al.hasAttribute("Alias"):
                aliasst = al.getAttribute("Alias")
                aliasnd = unicode(al.firstChild.data)
                aliases[aliasst] = aliasnd
    return aliases

class NodeSet(object):
    """ This class handles parsing XML description of namespaces, instantiating
        nodes, linking references, graphing the namespace and compiling a binary
        representation.

        Note that nodes assigned to this class are not restricted to having a
        single namespace ID. This class represents the entire physical address
        space of the binary representation and all nodes that are to be included
        in that segment of memory.
    """

    def __init__(self):
        self.nodes = {}
        self.aliases = {}
        self.namespaces = ["http://opcfoundation.org/UA/"]
        self.namespaceMapping = {}

    def sanitize(self):
        for n in self.nodes.values():
            if n.sanitize() == False:
                raise Exception("Failed to sanitize node " + str(n))

        # Sanitize reference consistency
        for n in self.nodes.values():
            for ref in n.references:
                if not ref.source == n.id:
                    raise Exception("Reference " + str(ref) + " has an invalid source")
                if not ref.referenceType in self.nodes:
                    raise Exception("Reference " + str(ref) + " has an unknown reference type")
                if not ref.target in self.nodes:
                    print(self.namespaces)
                    raise Exception("Reference " + str(ref) + " has an unknown target")

    def addNamespace(self, nsURL):
        if not nsURL in self.namespaces:
            self.namespaces.append(nsURL)

    def createNamespaceMapping(self, orig_namespaces):
        """Creates a dict that maps from the nsindex in the original nodeset to the
           nsindex in the combined nodeset"""
        self.namespaceMapping = {}
        for index, name in enumerate(orig_namespaces):
            self.namespaceMapping[index] = self.namespaces.index(name)

    def getNodeByBrowseName(self, idstring):
        return next((n for n in self.nodes.values() if idstring == n.browseName.name), None)

    def getRoot(self):
        return self.getNodeByBrowseName("Root")

    def createNode(self, xmlelement, modelUri, hidden=False):
        ndtype = xmlelement.localName.lower()
        if ndtype[:2] == "ua":
            ndtype = ndtype[2:]

        node = None
        if ndtype == 'variable':
            node = VariableNode(xmlelement)
        if ndtype == 'object':
            node = ObjectNode(xmlelement)
        if ndtype == 'method':
            node = MethodNode(xmlelement)
        if ndtype == 'objecttype':
            node = ObjectTypeNode(xmlelement)
        if ndtype == 'variabletype':
            node = VariableTypeNode(xmlelement)
        if ndtype == 'methodtype':
            node = MethodNode(xmlelement)
        if ndtype == 'datatype':
            node = DataTypeNode(xmlelement)
        if ndtype == 'referencetype':
            node = ReferenceTypeNode(xmlelement)

        if node is None:
            return None

        node.modelUri = modelUri
        node.hidden = hidden
        return node

    def hide_node(self, nodeId, hidden=True):
        if not nodeId in self.nodes:
            return False
        node = self.nodes[nodeId]
        node.hidden = hidden
        return True

    def merge_dicts(self, *dict_args):
        """
        Given any number of dicts, shallow copy and merge into a new dict,
        precedence goes to key value pairs in latter dicts.
        """
        result = {}
        for dictionary in dict_args:
            result.update(dictionary)
        return result

    def getNodeByIDString(self, idStr):
        # Split id to namespace part and id part
        m = re.match("ns=([^;]+);(.*)", idStr)
        if m:
            ns = m.group(1)
            # Convert namespace uri to index
            if not ns.isdigit():
                if ns not in self.namespaces:
                    return None
                ns = self.namespaces.index(ns)
                idStr = "ns={};{}".format(ns, m.group(2))
        nodeId = NodeId(idStr)
        if not nodeId in self.nodes:
            return None
        return self.nodes[nodeId]

    def remove_node(self, node):

        def filterRef(r, rt):
            return (r.referenceType != rt.referenceType) or (not (
                    rt.target == node.id or rt.source == node.id
                ))

        for r in node.references:
            if r.target == node.id:
                if r.source not in self.nodes:
                    continue
                self.nodes[r.source].references = dict(filter(
                    lambda rt: filterRef(r, rt[0]), # filter only on key of each dict item
                    self.nodes[r.source].references.items()
                ))
            elif r.source == node.id:
                if r.target not in self.nodes:
                    continue
                self.nodes[r.target].references = dict(filter(
                    lambda rt: filterRef(r, rt[0]), # filter only on key of each dict item
                    self.nodes[r.target].references.items()
                ))
        del self.nodes[node.id]


    def addNodeSet(self, xmlfile, hidden=False, typesArray="UA_TYPES"):
        # Extract NodeSet DOM

        fileContent = xmlfile.read()
        # Remove BOM since the dom parser cannot handle it on Python 3 Windows
        if fileContent.startswith( codecs.BOM_UTF8 ):
            fileContent = fileContent.lstrip( codecs.BOM_UTF8 )
        if (sys.version_info >= (3, 0)):
            fileContent = fileContent.decode("utf-8")

        # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
        fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\g<1>\g<2>\g<3>>", fileContent)

        nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
        if len(nodesets) == 0 or len(nodesets) > 1:
            raise Exception(self, self.originXML + " contains no or more then 1 nodeset")
        nodeset = nodesets[0]


        # Extract the modelUri
        try:
            modelTag = nodeset.getElementsByTagName("Models")[0].getElementsByTagName("Model")[0]
            modelUri = modelTag.attributes["ModelUri"].nodeValue
        except Exception:
            # Ignore exception and try to use namespace array
            modelUri = None


        # Create the namespace mapping
        orig_namespaces = extractNamespaces(xmlfile)  # List of namespaces used in the xml file
        if modelUri is None and len(orig_namespaces) > 1:
            modelUri = orig_namespaces[1]

        if modelUri is None:
            raise Exception(self, self.originXML + " does not define the nodeset URI in Models/Model/ModelUri or NamespaceUris array.")

        for ns in orig_namespaces:
            self.addNamespace(ns)
        self.createNamespaceMapping(orig_namespaces) # mapping for this file

        # Extract the aliases
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            ndtype = nd.localName.lower()
            if 'aliases' in ndtype:
                self.aliases = self.merge_dicts(self.aliases, buildAliasList(nd))

        # Instantiate nodes
        newnodes = {}
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            node = self.createNode(nd, modelUri, hidden)
            if not node:
                continue
            node.replaceAliases(self.aliases)
            node.replaceNamespaces(self.namespaceMapping)
            node.typesArray = typesArray

            # Add the node the the global dict
            if node.id in self.nodes:
                raise Exception("XMLElement with duplicate ID " + str(node.id))
            self.nodes[node.id] = node
            newnodes[node.id] = node

        # Parse Datatypes in order to find out what the XML keyed values actually
        # represent.
        # Ex. <rpm>123</rpm> is not encodable
        #     only after parsing the datatypes, it is known that
        #     rpm is encoded as a double
        for n in newnodes.values():
            if isinstance(n, DataTypeNode):
                n.buildEncoding(self, namespaceMapping=self.namespaceMapping)

    def getBinaryEncodingIdForNode(self, nodeId):
        """
        The node should have a 'HasEncoding' forward reference which points to the encoding ids.
        These can be XML Encoding or Binary Encoding. Therefore we also need to check if the SymbolicName
        of the target node is "DefaultBinary"
        """
        node = self.nodes[nodeId]
        for ref in node.references:
            if ref.referenceType.ns == 0 and ref.referenceType.i == 38:
                refNode = self.nodes[ref.target]
                if refNode.symbolicName.value == "DefaultBinary":
                    return ref.target
        raise Exception("No DefaultBinary encoding defined for node " + str(nodeId))

    def allocateVariables(self):
        for n in self.nodes.values():
            if isinstance(n, VariableNode):
                n.allocateValue(self)

    def getBaseDataType(self, node):
        if node is None:
            return None
        if node.browseName.name not in opaque_type_mapping:
            return node
        for ref in node.references:
            if ref.isForward:
                continue
            if ref.referenceType.i == 45:
                return self.getBaseDataType(self.nodes[ref.target])
        return node

    def getNodeTypeDefinition(self, node):
        for ref in node.references:
            # 40 = HasTypeDefinition
            if ref.referenceType.i == 40 and ref.isForward:
                return self.nodes[ref.target]
        return None

    def getDataTypeNode(self, dataType):
        if isinstance(dataType, string_types):
            if not valueIsInternalType(dataType):
                logger.error("Not a valid dataType string: " + dataType)
                return None
            return self.nodes[NodeId(self.aliases[dataType])]
        if isinstance(dataType, NodeId):
            if dataType.i == 0:
                return None
            dataTypeNode = self.nodes[dataType]
            if not isinstance(dataTypeNode, DataTypeNode):
                logger.error("Node id " + str(dataType) + " is not reference a valid dataType.")
                return None
            return dataTypeNode
        return None

    def getRelevantOrderingReferences(self):
        relevant_types = set()
        relevant_types.update(getSubTypesOf(self, self.getNodeByBrowseName("HierarchicalReferences"), []))
        relevant_types.update(getSubTypesOf(self, self.getNodeByBrowseName("HasEncoding"), []))
        relevant_types.update(getSubTypesOf(self, self.getNodeByBrowseName("HasTypeDefinition"), []))
        return list(map(lambda x: x.id, relevant_types))

    def addInverseReferences(self):
        # Ensure that every reference has an inverse reference in the target
        for u in self.nodes.values():
            for ref in u.references.copy():
                back = Reference(ref.target, ref.referenceType, ref.source, not ref.isForward)
                self.nodes[ref.target].references[back] = None # dict key does not make a duplicate entry

    def setNodeParent(self):
        parentreftypes = getSubTypesOf(self, self.getNodeByBrowseName("HierarchicalReferences"))
        parentreftypes = list(map(lambda x: x.id, parentreftypes))

        for node in self.nodes.values():
            if node.id.ns == 0 and node.id.i in [78, 80, 84]:
                # ModellingRule, Root node do not have a parent
                continue

            parentref = node.getParentReference(parentreftypes)
            if parentref is not None:
                node.parent = self.nodes[parentref.target]
                if not node.parent:
                    raise RuntimeError("Node {}: Did not find parent node: ".format(str(node.id)))
                node.parentReference = self.nodes[parentref.referenceType]
            # Some nodes in the full nodeset do not have a parent. So accept this and do not show an error.
            #else:
            #    raise RuntimeError("Node {}: HierarchicalReference (or subtype of it) to parent node is missing.".format(str(node.id)))

    def generateParser(self, existing, infiles , bsdFile):
        self.all_files = []
        import_bsd = []
        type_bsd = []
        for xmlfile in existing:
            if xmlfile.name == "deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml":
                continue
            nodeset_base = open(xmlfile.name, "rb")
            #fileContent = xmlfile.read()
            fileContent = nodeset_base.read()
            # Remove BOM since the dom parser cannot handle it on python 3 windows
            if fileContent.startswith( codecs.BOM_UTF8 ):
                fileContent = fileContent.lstrip( codecs.BOM_UTF8 )
            if (sys.version_info >= (3, 0)):
                fileContent = fileContent.decode("utf-8")

            # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
            fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\g<1>\g<2>\g<3>>", fileContent)

            nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
            if len(nodesets) == 0 or len(nodesets) > 1:
                raise Exception("contains no or more then 1 nodeset")
            nodeset = nodesets[0]
            variableNodes = nodeset.getElementsByTagName("UAVariable")
            for nd in variableNodes:
                if (nd.hasAttribute("SymbolicName") and (re.match(r".*_BinarySchema", nd.attributes["SymbolicName"].nodeValue) or nd.attributes["SymbolicName"].nodeValue == "TypeDictionary_BinarySchema")) or (nd.hasAttribute("ParentNodeId") and not nd.hasAttribute("SymbolicName") and re.fullmatch(r"i=93", nd.attributes["ParentNodeId"].nodeValue)):
                    type_content = nd.getElementsByTagName("Value")[0].getElementsByTagName("ByteString")[0]
                    f = tempfile.NamedTemporaryFile(delete=False, suffix='.bsd')
                    f.write(base64.b64decode(type_content.firstChild.nodeValue))
                    f.flush()
                    self.all_files.append(f.name)
                    f.close()
                    bsd = "UA_TYPES#" + f.name
                    import_bsd.append(bsd)
        for xmlfile in infiles:
            nodeset_base = open(xmlfile.name, "rb")
            #fileContent = xmlfile.read()
            fileContent = nodeset_base.read()
            # Remove BOM since the dom parser cannot handle it on python 3 windows
            if fileContent.startswith( codecs.BOM_UTF8 ):
                fileContent = fileContent.lstrip( codecs.BOM_UTF8 )
            if (sys.version_info >= (3, 0)):
                fileContent = fileContent.decode("utf-8")

            # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
            fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\g<1>\g<2>\g<3>>", fileContent)

            nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
            if len(nodesets) == 0 or len(nodesets) > 1:
                raise Exception("contains no or more then 1 nodeset")
            nodeset = nodesets[0]
            variableNodes = nodeset.getElementsByTagName("UAVariable")
            for nd in variableNodes:
                if (nd.hasAttribute("SymbolicName") and (re.match(r".*_BinarySchema", nd.attributes["SymbolicName"].nodeValue) or nd.attributes["SymbolicName"].nodeValue == "TypeDictionary_BinarySchema")) or (nd.hasAttribute("ParentNodeId") and not nd.hasAttribute("SymbolicName") and re.fullmatch(r"i=93", nd.attributes["ParentNodeId"].nodeValue)):
                    type_content = nd.getElementsByTagName("Value")[0].getElementsByTagName("ByteString")[0]
                    f = tempfile.NamedTemporaryFile(suffix='.bsd')
                    f.write(base64.b64decode(type_content.firstChild.nodeValue))
                    f.flush()
                    f.seek(0)
                    bf = io.BufferedReader(f)
                    self.all_files.append(f.name)
                    tmp = io.TextIOWrapper(bf, 'UTF-8', newline='\n')
                    tmp.mode = 'r'
                    type_bsd.append(tmp)

        opaque_map = []
        selected_types = []
        type_csv = []
        no_builtin = True
        outname = "outname"

        for bsd in bsdFile:
            type_bsd.append(bsd)

        self.parser = CSVBSDTypeParser(opaque_map, selected_types, no_builtin, outname, import_bsd,
                                    type_bsd, type_csv, self.namespaces)
        self.parser.create_types()

        nodeset_base.close()
