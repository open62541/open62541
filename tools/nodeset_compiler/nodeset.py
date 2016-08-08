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
import logging; logger = logging.getLogger(__name__)

from datatypes import *
from nodes import *
from constants import *

####################
# Helper Functions #
####################

def extractNamespaces(xmlfile):
    # Extract a list of namespaces used. The first namespace is always
    # "http://opcfoundation.org/UA/". minidom gobbles up
    # <NamespaceUris></NamespaceUris> elements, without a decent way to reliably
    # access this dom2 <uri></uri> elements (only attribute xmlns= are accessible
    # using minidom). We need them for dereferencing though... This function
    # attempts to do just that.
    
    namespaces = ["http://opcfoundation.org/UA/"]
    infile = open(xmlfile.name)
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
  def __init__(self, name):
    self.nodes = {}
    self.namespaces = ["http://opcfoundation.org/UA/"]

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
    for index,name in enumerate(orig_namespaces):
      m[index] = self.namespaces.index(name)
    return m

  def getNodeByBrowseName(self, idstring):
    return next((n for n in self.nodes.values() if idstring==n.browseName.name), None)

  def getRoot(self):
    return self.getNodeByBrowseName("Root")

  def createNode(self, xmlelement, nsMapping, hidden=False):
    ndtype = xmlelement.tagName.lower()
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

    if node and hidden:
        node.hidden = True
        for ref in node.references:
            ref.hidden = True
        for ref in node.inverseReferences:
            ref.hidden = True
    return node

  def addNodeSet(self, xmlfile, existing = False):
    # Extract NodeSet DOM
    nodesets = dom.parse(xmlfile).getElementsByTagName("UANodeSet")
    if len(nodesets) == 0 or len(nodesets) > 1:
      raise Exception(self, self.originXML + " contains no or more then 1 nodeset")
    nodeset = nodesets[0]

    # Create the namespace mapping
    orig_namespaces = extractNamespaces(xmlfile) # List of namespaces used in the xml file
    for ns in orig_namespaces:
        self.addNamespace(ns)
    nsMapping = self.createNamespaceMapping(orig_namespaces)

    # Extract the aliases
    aliases = None
    for nd in nodeset.childNodes:
      if nd.nodeType != nd.ELEMENT_NODE:
        continue
      ndtype = nd.tagName.lower()
      if 'aliases' in ndtype:
        aliases = buildAliasList(nd)

    # Instantiate nodes
    newnodes = []
    for nd in nodeset.childNodes:
      if nd.nodeType != nd.ELEMENT_NODE:
        continue
      node = self.createNode(nd, nsMapping)
      if not node:
        continue
      node.replaceAliases(aliases)
      node.replaceNamespaces(nsMapping)
      
      # Add the node the the global dict
      if node.id in self.nodes:
        raise Exception("XMLElement with duplicate ID " + str(node.id))
      self.nodes[node.id] = node
      newnodes.append(node)

    # add inverse references
    for node in newnodes:
        for ref in node.references:
            newsource = self.nodes[ref.target]
            hide = node.hidden and newsource.hidden
            newref = Reference(newsource.id, ref.referenceType, ref.source, False, hide)
            newsource.inverseReferences.add(newref)
        for ref in node.inverseReferences:
            newsource = self.nodes[ref.target]
            hide = node.hidden and newsource.hidden
            newref = Reference(newsource.id, ref.referenceType, ref.source, True, hide)
            newsource.references.add(newref)

        
