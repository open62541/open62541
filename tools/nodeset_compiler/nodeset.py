#!/usr/bin/env/python
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

from __future__ import print_function
import sys
import xml.dom.minidom as dom
from struct import pack as structpack
from time import struct_time, strftime, strptime, mktime
import logging
import codecs
import re

logger = logging.getLogger(__name__)

from nodes import *
from opaque_type_mapping import opaque_type_mapping
import codecs

####################
# Helper Functions #
####################

hassubtype = NodeId("ns=0;i=45")

def getSubTypesOf(nodeset, node, skipNodes=[]):
    if node in skipNodes:
        return []
    re = [node]
    for ref in node.references:
        if ref.referenceType == hassubtype and ref.isForward:
            re = re + getSubTypesOf(nodeset, nodeset.nodes[ref.target], skipNodes=skipNodes)
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
    line = infile.readline()
    for line in infile:
        if "<namespaceuris>" in line.lower():
            foundURIs = True
        elif "</namespaceuris>" in line.lower():
            foundURIs = False
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
        self.namespaceMapping = {};

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
                    raise Exception("Reference " + str(ref) + " has an unknown target")

    def addNamespace(self, nsURL):
        if not nsURL in self.namespaces:
            self.namespaces.append(nsURL)

    def createNamespaceMapping(self, orig_namespaces):
        """Creates a dict that maps from the nsindex in the original nodeset to the
           nsindex in the combined nodeset"""
        m = {}
        for index, name in enumerate(orig_namespaces):
            m[index] = self.namespaces.index(name)
        return m

    def getNodeByBrowseName(self, idstring):
        return next((n for n in self.nodes.values() if idstring == n.browseName.name), None)

    def getNodeById(self, namespace, id):
        nodeId = NodeId()
        nodeId.ns = namespace
        nodeId.i = id
        return self.nodes[nodeId]

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

        if hidden:
            # References from an existing nodeset are all suppressed
            for ref in node.references:
                ref.hidden = True
            for ref in node.inverseReferences:
                ref.hidden = True
        return node

    def hide_node(self, nodeId, hidden=True):
        if not nodeId in self.nodes:
            return False
        node = self.nodes[nodeId]
        node.hidden = hidden
        # References from an existing nodeset are all suppressed
        for ref in node.references:
            ref.hidden = hidden
        for ref in node.inverseReferences:
            ref.hidden = hidden
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

    def addNodeSet(self, xmlfile, hidden=False, typesArray="UA_TYPES"):
        # Extract NodeSet DOM

        fileContent = xmlfile.read()
        # Remove BOM since the dom parser cannot handle it on python 3 windows
        if fileContent.startswith( codecs.BOM_UTF8 ):
            fileContent = fileContent.lstrip( codecs.BOM_UTF8 )
        if (sys.version_info >= (3, 0)):
            fileContent = fileContent.decode("utf-8")

        # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
        fileContent = re.sub(r"<([/]?)uax:(\w+)([/]?)>", "<\g<1>\g<2>\g<3>>", fileContent)

        nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
        if len(nodesets) == 0 or len(nodesets) > 1:
            raise Exception(self, self.originXML + " contains no or more then 1 nodeset")
        nodeset = nodesets[0]


        # Extract the modelUri
        modelUri = None
        try:
            modelTag = nodeset.getElementsByTagName("Models")[0].getElementsByTagName("Model")[0]
            modelUri = modelTag.attributes["ModelUri"].nodeValue
        except:
            # Ignore exception and try to use namespace array
            modelUri = None


        # Create the namespace mapping
        orig_namespaces = extractNamespaces(xmlfile)  # List of namespaces used in the xml file
        if modelUri is None and len(orig_namespaces) > 0:
            modelUri = orig_namespaces[0]

        if modelUri is None:
            raise Exception(self, self.originXML + " does not define the nodeset URI in Models/Model/ModelUri or NamespaceUris array.")

        for ns in orig_namespaces:
            self.addNamespace(ns)
        self.namespaceMapping[modelUri] = self.createNamespaceMapping(orig_namespaces)

        # Extract the aliases
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            ndtype = nd.localName.lower()
            if 'aliases' in ndtype:
                self.aliases = self.merge_dicts(self.aliases, buildAliasList(nd))

        # Instantiate nodes
        newnodes = []
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            node = self.createNode(nd, modelUri, hidden)
            if not node:
                continue
            node.replaceAliases(self.aliases)
            node.replaceNamespaces(self.namespaceMapping[modelUri])
            node.typesArray = typesArray

            # Add the node the the global dict
            if node.id in self.nodes:
                raise Exception("XMLElement with duplicate ID " + str(node.id))
            self.nodes[node.id] = node
            newnodes.append(node)

        # add inverse references
        for node in newnodes:
            for ref in node.references:
                newsource = self.nodes[ref.target]
                hide = ref.hidden or (node.hidden and newsource.hidden)
                newref = Reference(newsource.id, ref.referenceType, ref.source, False, hide, inferred=True)
                newsource.inverseReferences.add(newref)
            for ref in node.inverseReferences:
                newsource = self.nodes[ref.target]
                hide = ref.hidden or (node.hidden and newsource.hidden)
                newref = Reference(newsource.id, ref.referenceType, ref.source, True, hide, inferred=True)
                newsource.references.add(newref)

    def getBinaryEncodingIdForNode(self, nodeId):
        """
        The node should have a 'HasEncoding' forward reference which points to the encoding ids.
        These can be XML Encoding or Binary Encoding. Therefore we also need to check if the SymbolicName
        of the target node is "DefaultBinary"
        """
        node = self.nodes[nodeId]
        refId = NodeId()
        for ref in node.references:
            if ref.referenceType.ns == 0 and ref.referenceType.i == 38:
                refNode = self.nodes[ref.target]
                if refNode.symbolicName.value == "DefaultBinary":
                    return ref.target
        raise Exception("No DefaultBinary encoding defined for node " + str(nodeId))

    def buildEncodingRules(self):
        """ Calls buildEncoding() for all DataType nodes (opcua_node_dataType_t).

            No return value
        """
        stat = {True: 0, False: 0}
        for n in self.nodes.values():
            if isinstance(n, DataTypeNode):
                n.buildEncoding(self)
                stat[n.isEncodable()] = stat[n.isEncodable()] + 1
        logger.debug("Type definitions built/passed: " +  str(stat))


    def allocateVariables(self):
        for n in self.nodes.values():
            if isinstance(n, VariableNode):
                n.allocateValue(self)


    def getBaseDataType(self, node):
        if node is None:
            return None
        if node.browseName.name not in opaque_type_mapping:
            return node
        for ref in node.inverseReferences:
            if ref.referenceType.i == 45:
                return self.getBaseDataType(self.nodes[ref.target])
        return node
                
    def getDataTypeNode(self, dataType):
        if isinstance(dataType, six.string_types):
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
            if not dataTypeNode.isEncodable():
                logger.warn("DataType " + str(dataTypeNode.browseName) + " is not encodable.")
            return dataTypeNode
        return None

    def getRelevantOrderingReferences(self):
        relevant_types = getSubTypesOf(self,
                                       self.getNodeByBrowseName("HierarchicalReferences"),
                                       [])
        relevant_types += getSubTypesOf(self,
                                        self.getNodeByBrowseName("HasEncoding"),
                                        [])
        relevant_types = map(lambda x: x.id, relevant_types)
        return list(relevant_types)
