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
from time import struct_time, strftime, strptime, mktime
from struct import pack as structpack

import logging
from ua_builtin_types import *;
from ua_node_types import *;
from ua_constants import *;
from open62541_MacroHelper import open62541_MacroHelper


logger = logging.getLogger(__name__)

def getNextElementNode(xmlvalue):
  if xmlvalue == None:
    return None
  xmlvalue = xmlvalue.nextSibling
  while not xmlvalue == None and not xmlvalue.nodeType == xmlvalue.ELEMENT_NODE:
    xmlvalue = xmlvalue.nextSibling
  return xmlvalue

###
### Namespace Organizer
###

class opcua_namespace():
  """ Class holding and managing a set of OPCUA nodes.

      This class handles parsing XML description of namespaces, instantiating
      nodes, linking references, graphing the namespace and compiling a binary
      representation.

      Note that nodes assigned to this class are not restricted to having a
      single namespace ID. This class represents the entire physical address
      space of the binary representation and all nodes that are to be included
      in that segment of memory.
  """
  nodes = []
  nodeids = {}
  aliases = {}
  __linkLater__ = []
  __binaryIndirectPointers__ = []
  name = ""
  knownNodeTypes = ""
  namespaceIdentifiers = {} # list of 'int':'string' giving different namespace an array-mapable name

  def __init__(self, name):
    self.nodes = []
    self.knownNodeTypes = ['variable', 'object', 'method', 'referencetype', \
                           'objecttype', 'variabletype', 'methodtype', \
                           'datatype', 'referencetype', 'aliases']
    self.name = name
    self.nodeids = {}
    self.aliases = {}
    self.namespaceIdentifiers = {}
    self.__binaryIndirectPointers__ = []

  def addNamespace(self, numericId, stringURL):
    self.namespaceIdentifiers[numericId] = stringURL

  def linkLater(self, pointer):
    """ Called by nodes or references who have parsed an XML reference to a
        node represented by a string.

        No return value

        XML String representations of references have the form 'i=xy' or
        'ns=1;s="This unique Node"'. Since during the parsing of this attribute
        only a subset of nodes are known/parsed, this reference string cannot be
        linked when encountered.

        References register themselves with the namespace to have their target
        attribute (string) parsed by linkOpenPointers() when all nodes are
        created, so that target can be dereferenced an point to an actual node.
    """
    self.__linkLater__.append(pointer)

  def getUnlinkedPointers(self):
    """ Return the list of references registered for linking during the next call
        of linkOpenPointers()
    """
    return self.__linkLater__

  def unlinkedItemCount(self):
    """ Returns the number of unlinked references that will be processed during
        the next call of linkOpenPointers()
    """
    return len(self.__linkLater__)

  def buildAliasList(self, xmlelement):
    """ Parses the <Alias> XML Element present in must XML NodeSet definitions.

        No return value

        Contents the Alias element are stored in a dictionary for further
        dereferencing during pointer linkage (see linkOpenPointer()).
    """
    if not xmlelement.tagName == "Aliases":
      logger.error("XMLElement passed is not an Aliaslist")
      return
    for al in xmlelement.childNodes:
      if al.nodeType == al.ELEMENT_NODE:
        if al.hasAttribute("Alias"):
          aliasst = al.getAttribute("Alias")
          if sys.version_info[0] < 3:
            aliasnd = unicode(al.firstChild.data)
          else:
            aliasnd = al.firstChild.data
          if not aliasst in self.aliases:
            self.aliases[aliasst] = aliasnd
            logger.debug("Added new alias \"" + str(aliasst) + "\" == \"" + str(aliasnd) + "\"")
          else:
            if self.aliases[aliasst] != aliasnd:
              logger.error("Alias definitions for " + aliasst + " differ. Have " + self.aliases[aliasst] + " but XML defines " + aliasnd + ". Keeping current definition.")

  def getNodeByBrowseName(self, idstring):
    """ Returns the first node in the nodelist whose browseName matches idstring.
    """
    matches = []
    for n in self.nodes:
      if idstring==str(n.browseName()):
        matches.append(n)
    if len(matches) > 1:
      logger.error("Found multiple nodes with same ID!?")
    if len(matches) == 0:
      return None
    else:
      return matches[0]

  def getNodeByIDString(self, idstring):
    """ Returns the first node in the nodelist whose id string representation
        matches idstring.
    """
    matches = []
    for n in self.nodes:
      if idstring==str(n.id()):
        matches.append(n)
    if len(matches) > 1:
      logger.error("Found multiple nodes with same ID!?")
    if len(matches) == 0:
      return None
    else:
      return matches[0]

  def createNode(self, ndtype, xmlelement):
    """ createNode is instantiates a node described by xmlelement, its type being
        defined by the string ndtype.

        No return value

        If the xmlelement is an <Alias>, the contents will be parsed and stored
        for later dereferencing during pointer linking (see linkOpenPointers).

        Recognized types are:
        * UAVariable
        * UAObject
        * UAMethod
        * UAView
        * UAVariableType
        * UAObjectType
        * UAMethodType
        * UAReferenceType
        * UADataType

        For every recognized type, an appropriate node class is added to the node
        list of the namespace. The NodeId of the given node is created and parsing
        of the node attributes and elements is delegated to the parseXML() and
        parseXMLSubType() functions of the instantiated class.

        If the NodeID attribute is non-unique in the node list, the creation is
        deferred and an error is logged.
    """
    if not isinstance(xmlelement, dom.Element):
      logger.error( "Error: Can not create node from invalid XMLElement")
      return

    # An ID is mandatory for everything but aliases!
    id = None
    for idname in ['NodeId', 'NodeID', 'nodeid']:
      if xmlelement.hasAttribute(idname):
        id = xmlelement.getAttribute(idname)
    if ndtype == 'aliases':
      self.buildAliasList(xmlelement)
      return
    elif id == None:
      logger.info( "Error: XMLElement has no id, node will not be created!")
      return
    else:
      id = opcua_node_id_t(id)

    if str(id) in self.nodeids:
      # Normal behavior: Do not allow duplicates, first one wins
      #logger.error( "XMLElement with duplicate ID " + str(id) + " found, node will not be created!")
      #return
      # Open62541 behavior for header generation: Replace the duplicate with the new node
      logger.info( "XMLElement with duplicate ID " + str(id) + " found, node will be replaced!")
      nd = self.getNodeByIDString(str(id))
      self.nodes.remove(nd)
      self.nodeids.pop(str(nd.id()))

    node = None
    if (ndtype == 'variable'):
      node = opcua_node_variable_t(id, self)
    elif (ndtype == 'object'):
      node = opcua_node_object_t(id, self)
    elif (ndtype == 'method'):
      node = opcua_node_method_t(id, self)
    elif (ndtype == 'objecttype'):
      node = opcua_node_objectType_t(id, self)
    elif (ndtype == 'variabletype'):
      node = opcua_node_variableType_t(id, self)
    elif (ndtype == 'methodtype'):
      node = opcua_node_methodType_t(id, self)
    elif (ndtype == 'datatype'):
      node = opcua_node_dataType_t(id, self)
    elif (ndtype == 'referencetype'):
      node = opcua_node_referenceType_t(id, self)
    else:
      logger.error( "No node constructor for type " + ndtype)

    if node != None:
      node.parseXML(xmlelement)

    self.nodes.append(node)
    self.nodeids[str(node.id())] = node

  def removeNodeById(self, nodeId):
    nd = self.getNodeByIDString(nodeId)
    if nd == None:
      return False

    logger.debug("Removing nodeId " + str(nodeId))
    self.nodes.remove(nd)
    if nd.getInverseReferences() != None:
      for ref in nd.getInverseReferences():
        src = ref.target();
        src.removeReferenceToNode(nd)

    return True

  def registerBinaryIndirectPointer(self, node):
    """ Appends a node to the list of nodes that should be contained in the
        first 765 bytes (255 pointer slots a 3 bytes) in the binary
        representation (indirect referencing space).

        This function is reserved for references and dataType pointers.
    """
    if not node in self.__binaryIndirectPointers__:
      self.__binaryIndirectPointers__.append(node)
    return self.__binaryIndirectPointers__.index(node)

  def getBinaryIndirectPointerIndex(self, node):
    """ Returns the slot/index of a pointer in the indirect referencing space
        (first 765 Bytes) of the binary representation.
    """
    if not node in self.__binaryIndirectPointers__:
      return -1
    return self.__binaryIndirectPointers__.index(node)


  def parseXML(self, xmldoc):
    """ Reads an XML Namespace definition and instantiates node.

        No return value

        parseXML open the file xmldoc using xml.dom.minidom and searches for
        the first UANodeSet Element. For every Element encountered, createNode
        is called to instantiate a node of the appropriate type.
    """
    typedict = {}
    UANodeSet = dom.parse(xmldoc).getElementsByTagName("UANodeSet")
    if len(UANodeSet) == 0:
      logger.error( "Error: No NodeSets found")
      return
    if len(UANodeSet) != 1:
      logger.error( "Error: Found more than 1 Nodeset in XML File")

    UANodeSet = UANodeSet[0]
    for nd in UANodeSet.childNodes:
      if nd.nodeType != nd.ELEMENT_NODE:
        continue

      ndType = nd.tagName.lower()
      if ndType[:2] == "ua":
        ndType = ndType[2:]
      elif not ndType in self.knownNodeTypes:
        logger.warn("XML Element or NodeType " + ndType + " is unknown and will be ignored")
        continue

      if not ndType in typedict:
        typedict[ndType] = 1
      else:
        typedict[ndType] = typedict[ndType] + 1

      self.createNode(ndType, nd)
    logger.debug("Currently " + str(len(self.nodes)) + " nodes in address space. Type distribution for this run was: " + str(typedict))

  def linkOpenPointers(self):
    """ Substitutes symbolic NodeIds in references for actual node instances.

        No return value

        References that have registered themselves with linkLater() to have
        their symbolic NodeId targets ("ns=2;i=32") substituted for an actual
        node will be iterated by this function. For each reference encountered
        in the list of unlinked/open references, the target string will be
        evaluated and searched for in the node list of this namespace. If found,
        the target attribute of the reference will be substituted for the
        found node.

        If a reference fails to get linked, it will remain in the list of
        unlinked references. The individual items in this list can be
        retrieved using getUnlinkedPointers().
    """
    linked = []

    logger.debug( str(self.unlinkedItemCount()) + " pointers need to get linked.")
    for l in self.__linkLater__:
      targetLinked = False
      if not l.target() == None and not isinstance(l.target(), opcua_node_t):
        if isinstance(l.target(),str) or isinstance(l.target(),unicode):
          # If is not a node ID, it should be an alias. Try replacing it
          # with a proper node ID
          if l.target() in self.aliases:
            l.target(self.aliases[l.target()])
          # If the link is a node ID, try to find it hopening that no ass has
          # defined more than one kind of id for that sucker
          if l.target()[:2] == "i=" or l.target()[:2] == "g=" or \
             l.target()[:2] == "b=" or l.target()[:2] == "s=" or \
             l.target()[:3] == "ns=" :
            tgt = self.getNodeByIDString(str(l.target()))
            if tgt == None:
              logger.error("Failed to link pointer to target (node not found) " + l.target())
            else:
              l.target(tgt)
              targetLinked = True
          else:
            logger.error("Failed to link pointer to target (target not Alias or Node) " + l.target())
        else:
          logger.error("Failed to link pointer to target (don't know dummy type + " + str(type(l.target())) + " +) " + str(l.target()))
      else:
        logger.error("Pointer has null target: " + str(l))


      referenceLinked = False
      if not l.referenceType() == None:
        if l.referenceType() in self.aliases:
          l.referenceType(self.aliases[l.referenceType()])
        tgt = self.getNodeByIDString(str(l.referenceType()))
        if tgt == None:
          logger.error("Failed to link reference type to target (node not found) " + l.referenceType())
        else:
          l.referenceType(tgt)
          referenceLinked = True
      else:
        referenceLinked = True

      if referenceLinked == True and targetLinked == True:
        linked.append(l)

    # References marked as "not forward" must be inverted (removed from source node, assigned to target node and relinked)
    logger.warn("Inverting reference direction for all references with isForward==False attribute (is this correct!?)")
    for n in self.nodes:
      for r in n.getReferences():
        if r.isForward() == False:
          tgt = r.target()
          if isinstance(tgt, opcua_node_t):
            nref = opcua_referencePointer_t(n, parentNode=tgt)
            nref.referenceType(r.referenceType())
            tgt.addReference(nref)

    # Create inverse references for all nodes
    logger.debug("Updating all referencedBy fields in nodes for inverse lookups.")
    for n in self.nodes:
      n.updateInverseReferences()

    for l in linked:
      self.__linkLater__.remove(l)

    if len(self.__linkLater__) != 0:
      logger.warn(str(len(self.__linkLater__)) + " could not be linked.")

  def sanitize(self):
    remove = []
    logger.debug("Sanitizing nodes and references...")
    for n in self.nodes:
      if n.sanitize() == False:
        remove.append(n)
    if not len(remove) == 0:
      logger.warn(str(len(remove)) + " nodes will be removed because they failed sanitation.")
    # FIXME: Some variable ns=0 nodes fail because they don't have DataType fields...
    #        How should this be handles!?
    logger.warn("Not actually removing nodes... it's unclear if this is valid or not")

  def getRoot(self):
    """ Returns the first node instance with the browseName "Root".
    """
    return self.getNodeByBrowseName("Root")

  def buildEncodingRules(self):
    """ Calls buildEncoding() for all DataType nodes (opcua_node_dataType_t).

        No return value
    """
    stat = {True: 0, False: 0}
    for n in self.nodes:
      if isinstance(n, opcua_node_dataType_t):
        n.buildEncoding()
        stat[n.isEncodable()] = stat[n.isEncodable()] + 1
    logger.debug("Type definitions built/passed: " +  str(stat))

  def allocateVariables(self):
    for n in self.nodes:
      if isinstance(n, opcua_node_variable_t):
        n.allocateValue()

  def printDot(self, filename="namespace.dot"):
    """ Outputs a graphiz/dot description of all nodes in the namespace.

        Output will written into filename to be parsed by dot/neato...

        Note that for namespaces with more then 20 nodes the reference structure
        will lead to a mostly illegible and huge graph. Use printDotGraphWalk()
        for plotting specific portions of a large namespace.
    """
    file=open(filename, 'w+')

    file.write("digraph ns {\n")
    for n in self.nodes:
      file.write(n.printDot())
    file.write("}\n")
    file.close()

  def getSubTypesOf(self, tdNodes = None, currentNode = None, hasSubtypeRefNode = None):
    # If this is a toplevel call, collect the following information as defaults
    if tdNodes == None:
      tdNodes = []
    if currentNode == None:
      currentNode = self.getNodeByBrowseName("HasTypeDefinition")
      tdNodes.append(currentNode)
      if len(tdNodes) < 1:
        return []
    if hasSubtypeRefNode == None:
      hasSubtypeRefNode = self.getNodeByBrowseName("HasSubtype")
      if hasSubtypeRefNode == None:
        return tdNodes

    # collect all subtypes of this node
    for ref in currentNode.getReferences():
      if ref.isForward() and ref.referenceType().id() == hasSubtypeRefNode.id():
        tdNodes.append(ref.target())
        self.getTypeDefinitionNodes(tdNodes=tdNodes, currentNode = ref.target(), hasSubtypeRefNode=hasSubtypeRefNode)

    return tdNodes


  def printDotGraphWalk(self, depth=1, filename="out.dot", rootNode=None, followInverse = False, excludeNodeIds=[]):
    """ Outputs a graphiz/dot description the nodes centered around rootNode.

        References beginning from rootNode will be followed for depth steps. If
        "followInverse = True" is passed, then inverse (not Forward) references
        will also be followed.

        Nodes can be excluded from the graph by passing a list of NodeIds as
        string representation using excludeNodeIds (ex ["i=53", "ns=2;i=453"]).

        Output is written into filename to be parsed by dot/neato/srfp...
    """
    iter = depth
    processed = []
    if rootNode == None or \
       not isinstance(rootNode, opcua_node_t) or \
       not rootNode in self.nodes:
      root = self.getRoot()
    else:
      root = rootNode

    file=open(filename, 'w+')

    if root == None:
      return

    file.write("digraph ns {\n")
    file.write(root.printDot())
    refs=[]
    if followInverse == True:
      refs = root.getReferences(); # + root.getInverseReferences()
    else:
      for ref in root.getReferences():
        if ref.isForward():
          refs.append(ref)
    while iter > 0:
      tmp = []
      for ref in refs:
        if isinstance(ref.target(), opcua_node_t):
          tgt = ref.target()
          if not str(tgt.id()) in excludeNodeIds:
            if not tgt in processed:
              file.write(tgt.printDot())
              processed.append(tgt)
              if ref.isForward() == False and followInverse == True:
                tmp = tmp + tgt.getReferences(); # + tgt.getInverseReferences()
              elif ref.isForward() == True :
                tmp = tmp + tgt.getReferences();
      refs = tmp
      iter = iter - 1

    file.write("}\n")
    file.close()

  def __reorder_getMinWeightNode__(self, nmatrix):
    rcind = -1
    rind = -1
    minweight = -1
    minweightnd = None
    for row in nmatrix:
      rcind += 1
      if row[0] == None:
        continue
      w = sum(row[1:])
      if minweight < 0:
        rind = rcind
        minweight = w
        minweightnd = row[0]
      elif w < minweight:
        rind = rcind
        minweight = w
        minweightnd = row[0]
    return (rind, minweightnd, minweight)

  def reorderNodesMinDependencies(self):
    # create a matrix represtantion of all node
    #
    nmatrix = []
    for n in range(0,len(self.nodes)):
      nmatrix.append([None] + [0]*len(self.nodes))

    typeRefs = []
    tn = self.getNodeByBrowseName("HasTypeDefinition")
    if tn != None:
      typeRefs.append(tn)
      typeRefs = typeRefs + self.getSubTypesOf(currentNode=tn)
    subTypeRefs = []
    tn = self.getNodeByBrowseName("HasSubtype")
    if tn  != None:
      subTypeRefs.append(tn)
      subTypeRefs = subTypeRefs + self.getSubTypesOf(currentNode=tn)

    logger.debug("Building connectivity matrix for node order optimization.")
    # Set column 0 to contain the node
    for node in self.nodes:
      nind = self.nodes.index(node)
      nmatrix[nind][0] = node

    # Determine the dependencies of all nodes
    for node in self.nodes:
      nind = self.nodes.index(node)
      #print "Examining node " + str(nind) + " " + str(node)
      for ref in node.getReferences():
        if isinstance(ref.target(), opcua_node_t):
          tind = self.nodes.index(ref.target())
          # Typedefinition of this node has precedence over this node
          if ref.referenceType() in typeRefs and ref.isForward():
            nmatrix[nind][tind+1] += 1
          # isSubTypeOf/typeDefinition of this node has precedence over this node
          elif ref.referenceType() in subTypeRefs and not ref.isForward():
            nmatrix[nind][tind+1] += 1
          # Else the target depends on us
          elif ref.isForward():
            nmatrix[tind][nind+1] += 1

    logger.debug("Using Djikstra topological sorting to determine printing order.")
    reorder = []
    while len(reorder) < len(self.nodes):
      (nind, node, w) = self.__reorder_getMinWeightNode__(nmatrix)
      #print  str(100*float(len(reorder))/len(self.nodes)) + "% " + str(w) + " " + str(node) + " " + str(node.browseName())
      reorder.append(node)
      for ref in node.getReferences():
        if isinstance(ref.target(), opcua_node_t):
          tind = self.nodes.index(ref.target())
          if ref.referenceType() in typeRefs and ref.isForward():
            nmatrix[nind][tind+1] -= 1
          elif ref.referenceType() in subTypeRefs and not ref.isForward():
            nmatrix[nind][tind+1] -= 1
          elif ref.isForward():
            nmatrix[tind][nind+1] -= 1
      nmatrix[nind][0] = None
    self.nodes = reorder
    logger.debug("Nodes reordered.")
    return

  def printOpen62541Header(self, printedExternally=[], supressGenerationOfAttribute=[], outfilename=""):
    unPrintedNodes = []
    unPrintedRefs  = []
    code = []
    header = []

    # Reorder our nodes to produce a bare minimum of bootstrapping dependencies
    logger.debug("Reordering nodes for minimal dependencies during printing.")
    self.reorderNodesMinDependencies()

    # Some macros (UA_EXPANDEDNODEID_MACRO()...) are easily created, but
    # bulky. This class will help to offload some code.
    codegen = open62541_MacroHelper(supressGenerationOfAttribute=supressGenerationOfAttribute)

    # Populate the unPrinted-Lists with everything we have.
    # Every Time a nodes printfunction is called, it will pop itself and
    #   all printed references from these lists.
    for n in self.nodes:
      if not n in printedExternally:
        unPrintedNodes.append(n)
      else:
        logger.debug("Node " + str(n.id()) + " is being ignored.")
    for n in unPrintedNodes:
      for r in n.getReferences():
        if (r.target() != None) and (r.target().id() != None) and (r.parent() != None):
          unPrintedRefs.append(r)

    logger.debug(str(len(unPrintedNodes)) + " Nodes, " + str(len(unPrintedRefs)) +  "References need to get printed.")
    header.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")
    code.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")

    header.append('#ifndef '+outfilename.upper()+'_H_')
    header.append('#define '+outfilename.upper()+'_H_')
    header.append('#ifdef UA_NO_AMALGAMATION')
    header.append('#include "server/ua_server_internal.h"')
    header.append('#include "server/ua_nodes.h"')
    header.append('#include "ua_util.h"')
    header.append('#include "ua_types.h"')
    header.append('#include "ua_types_encoding_binary.h"')
    header.append('#include "ua_types_generated_encoding_binary.h"')
    header.append('#include "ua_transport_generated_encoding_binary.h"')
    header.append('#else')
    header.append('#include "open62541.h"')
    header.append('#define NULL ((void *)0)')
    header.append('#endif')

    code.append('#include "'+outfilename+'.h"')
    code.append("UA_INLINE void "+outfilename+"(UA_Server *server) {")

    # Before printing nodes, we need to request additional namespace arrays from the server
    for nsid in self.namespaceIdentifiers:
      if nsid == 0 or nsid==1:
        continue
      else:
        name =  self.namespaceIdentifiers[nsid]
        name = name.replace("\"","\\\"")
        code.append("UA_Server_addNamespace(server, \"" + name + "\");")

    # Find all references necessary to create the namespace and
    # "Bootstrap" them so all other nodes can safely use these referencetypes whenever
    # they can locate both source and target of the reference.
    logger.debug("Collecting all references used in the namespace.")
    refsUsed = []
    for n in self.nodes:
      # Since we are already looping over all nodes, use this chance to print NodeId defines
      if n.id().ns != 0:
        nc = n.nodeClass()
        if nc != NODE_CLASS_OBJECT and nc != NODE_CLASS_VARIABLE and nc != NODE_CLASS_VIEW:
          header = header + codegen.getNodeIdDefineString(n)

      # Now for the actual references...
      for r in n.getReferences():
        # Only print valid references in namespace 0 (users will not want their refs bootstrapped)
        if not r.referenceType() in refsUsed and r.referenceType() != None and r.referenceType().id().ns == 0:
          refsUsed.append(r.referenceType())
    logger.debug(str(len(refsUsed)) + " reference types are used in the namespace, which will now get bootstrapped.")
    for r in refsUsed:
      code = code + r.printOpen62541CCode(unPrintedNodes, unPrintedRefs);

    header.append("extern void "+outfilename+"(UA_Server *server);\n")
    header.append("#endif /* "+outfilename.upper()+"_H_ */")

    # Note to self: do NOT - NOT! - try to iterate over unPrintedNodes!
    #               Nodes remove themselves from this list when printed.
    logger.debug("Printing all other nodes.")
    for n in self.nodes:
      code = code + n.printOpen62541CCode(unPrintedNodes, unPrintedRefs, supressGenerationOfAttribute=supressGenerationOfAttribute)

    if len(unPrintedNodes) != 0:
      logger.warn("" + str(len(unPrintedNodes)) + " nodes could not be translated to code.")
    else:
      logger.debug("Printing suceeded for all nodes")

    if len(unPrintedRefs) != 0:
      logger.debug("Attempting to print " + str(len(unPrintedRefs)) + " unprinted references.")
      tmprefs = []
      for r in unPrintedRefs:
        if  not (r.target() not in unPrintedNodes) and not (r.parent() in unPrintedNodes):
          if not isinstance(r.parent(), opcua_node_t):
            logger.debug("Reference has no parent!")
          elif not isinstance(r.parent().id(), opcua_node_id_t):
            logger.debug("Parents nodeid is not a nodeID!")
          else:
            if (len(tmprefs) == 0):
              code.append("//  Creating leftover references:")
            code = code + codegen.getCreateStandaloneReference(r.parent(), r)
            code.append("")
            tmprefs.append(r)
      # Remove printed refs from list
      for r in tmprefs:
        unPrintedRefs.remove(r)
      if len(unPrintedRefs) != 0:
        logger.warn("" + str(len(unPrintedRefs)) + " references could not be translated to code.")
    else:
      logger.debug("Printing succeeded for all references")

    code.append("}")
    return (header,code)

###
### Testing
###

class testing:
  def __init__(self):
    self.namespace = opcua_namespace("testing")

    logger.debug("Phase 1: Reading XML file nodessets")
    self.namespace.parseXML("Opc.Ua.NodeSet2.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.namespace.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    logger.debug("Phase 2: Linking address space references and datatypes")
    self.namespace.linkOpenPointers()
    self.namespace.sanitize()

    logger.debug("Phase 3: Comprehending DataType encoding rules")
    self.namespace.buildEncodingRules()

    logger.debug("Phase 4: Allocating variable value data")
    self.namespace.allocateVariables()

    bin = self.namespace.buildBinary()
    f = open("binary.base64","w+")
    f.write(bin.encode("base64"))
    f.close()

    allnodes = self.namespace.nodes;
    ns = [self.namespace.getRoot()]

    i = 0
    #print "Starting depth search on " + str(len(allnodes)) + " nodes starting with from " + str(ns)
    while (len(ns) < len(allnodes)):
      i = i + 1;
      tmp = [];
      print("Iteration: " + str(i))
      for n in ns:
        tmp.append(n)
        for r in n.getReferences():
          if (not r.target() in tmp):
           tmp.append(r.target())
      print("...tmp, " + str(len(tmp)) + " nodes discovered")
      ns = []
      for n in tmp:
        ns.append(n)
      print("...done, " + str(len(ns)) + " nodes discovered")

    logger.debug("Phase 5: Printing pretty graph")
    self.namespace.printDotGraphWalk(depth=1, rootNode=self.namespace.getNodeByIDString("i=84"), followInverse=False, excludeNodeIds=["i=29", "i=22", "i=25"])
    #self.namespace.printDot()

class testing_open62541_header:
  def __init__(self):
    self.namespace = opcua_namespace("testing")

    logger.debug("Phase 1: Reading XML file nodessets")
    self.namespace.parseXML("Opc.Ua.NodeSet2.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.namespace.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    logger.debug("Phase 2: Linking address space references and datatypes")
    self.namespace.linkOpenPointers()
    self.namespace.sanitize()

    logger.debug("Phase 3: Calling C Printers")
    code = self.namespace.printOpen62541Header()

    codeout = open("./open62541_namespace.c", "w+")
    for line in code:
      codeout.write(line + "\n")
    codeout.close()
    return

# Call testing routine if invoked standalone.
# For better debugging, it is advised to import this file using an interactive
# python shell and instantiating a namespace.
#
# import ua_types.py as ua; ns=ua.testing().namespace
if __name__ == '__main__':
  tst = testing_open62541_header()
