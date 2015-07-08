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


from time import struct_time, strftime, strptime, mktime
from struct import pack as structpack

from logger import *;
from ua_builtin_types import *;
from ua_node_types import *;
from open62541_MacroHelper import open62541_MacroHelper

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

  def __init__(self, name):
    self.nodes = []
    self.knownNodeTypes = ['variable', 'object', 'method', 'referencetype', \
                           'objecttype', 'variabletype', 'methodtype', \
                           'datatype', 'referencetype', 'aliases']
    self.name = name
    self.nodeids = {}
    self.aliases = {}
    self.__binaryIndirectPointers__ = []

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
      log(self, "XMLElement passed is not an Aliaslist", LOG_LEVEL_ERROR)
      return
    for al in xmlelement.childNodes:
      if al.nodeType == al.ELEMENT_NODE:
        if al.hasAttribute("Alias"):
          aliasst = al.getAttribute("Alias")
          aliasnd = unicode(al.firstChild.data)
          if not self.aliases.has_key(aliasst):
            self.aliases[aliasst] = aliasnd
            log(self, "Added new alias \"" + str(aliasst) + "\" == \"" + str(aliasnd) + "\"")
          else:
            if self.aliases[aliasst] != aliasnd:
              log(self, "Alias definitions for " + aliasst + " differ. Have " + self.aliases[aliasst] + " but XML defines " + aliasnd + ". Keeping current definition.", LOG_LEVEL_ERROR)

  def getNodeByBrowseName(self, idstring):
    """ Returns the first node in the nodelist whose browseName matches idstring.
    """
    matches = []
    for n in self.nodes:
      if idstring==str(n.browseName()):
        matches.append(n)
    if len(matches) > 1:
      log(self, "Found multiple nodes with same ID!?", LOG_LEVEL_ERROR)
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
      log(self, "Found multiple nodes with same ID!?", LOG_LEVEL_ERROR)
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
      log(self,  "Error: Can not create node from invalid XMLElement", LOG_LEVEL_ERROR)
      return

    # An ID is manditory for everything but aliases!
    id = None
    for idname in ['NodeId', 'NodeID', 'nodeid']:
      if xmlelement.hasAttribute(idname):
        id = xmlelement.getAttribute(idname)
    if ndtype == 'aliases':
      self.buildAliasList(xmlelement)
      return
    elif id == None:
      log(self,  "Error: XMLElement has no id, node will not be created!", LOG_LEVEL_INFO)
      return
    else:
      id = opcua_node_id_t(id)

    if self.nodeids.has_key(str(id)):
      # Normal behavior: Do not allow duplicates, first one wins
      #log(self,  "XMLElement with duplicate ID " + str(id) + " found, node will not be created!", LOG_LEVEL_ERROR)
      #return
      # Open62541 behavior for header generation: Replace the duplicate with the new node
      log(self,  "XMLElement with duplicate ID " + str(id) + " found, node will be replaced!", LOG_LEVEL_INFO)
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
      log(self,  "No node constructor for type " + ndtype, LOG_LEVEL_ERROR)

    if node != None:
      node.parseXML(xmlelement)

    self.nodes.append(node)
    self.nodeids[str(node.id())] = node

  def removeNodeById(self, nodeId):
    nd = self.getNodeByIDString(nodeId)
    if nd == None:
      return False

    log(self, "Removing nodeId " + str(nodeId), LOG_LEVEL_DEBUG)
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
      log(self,  "Error: No NodeSets found", LOG_LEVEL_ERROR)
      return
    if len(UANodeSet) != 1:
      log(self,  "Error: Found more than 1 Nodeset in XML File", LOG_LEVEL_ERROR)

    UANodeSet = UANodeSet[0]
    for nd in UANodeSet.childNodes:
      if nd.nodeType != nd.ELEMENT_NODE:
        continue

      ndType = nd.tagName.lower()
      if ndType[:2] == "ua":
        ndType = ndType[2:]
      elif not ndType in self.knownNodeTypes:
        log(self, "XML Element or NodeType " + ndType + " is unknown and will be ignored", LOG_LEVEL_WARN)
        continue

      if not typedict.has_key(ndType):
        typedict[ndType] = 1
      else:
        typedict[ndType] = typedict[ndType] + 1

      self.createNode(ndType, nd)
    log(self, "Currently " + str(len(self.nodes)) + " nodes in address space. Type distribution for this run was: " + str(typedict))

  def linkOpenPointers(self):
    """ Substitutes symbolic NodeIds in references for actual node instances.

        No return value

        References that have registered themselves with linkLater() to have
        their symbolic NodeId targets ("ns=2; i=32") substited for an actual
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

    log(self,  str(self.unlinkedItemCount()) + " pointers need to get linked.")
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
              log(self, "Failed to link pointer to target (node not found) " + l.target(), LOG_LEVEL_ERROR)
            else:
              l.target(tgt)
              targetLinked = True
          else:
            log(self, "Failed to link pointer to target (target not Alias or Node) " + l.target(), LOG_LEVEL_ERROR)
        else:
          log(self, "Failed to link pointer to target (don't know dummy type + " + str(type(l.target())) + " +) " + str(l.target()), LOG_LEVEL_ERROR)
      else:
        log(self, "Pointer has null target: " + str(l), LOG_LEVEL_ERROR)


      referenceLinked = False
      if not l.referenceType() == None:
        if l.referenceType() in self.aliases:
          l.referenceType(self.aliases[l.referenceType()])
        if l.referenceType()[:2] == "i=" or l.referenceType()[:2] == "g=" or \
           l.referenceType()[:2] == "b=" or l.referenceType()[:2] == "s=":
          tgt = self.getNodeByIDString(str(l.referenceType()))
          if tgt == None:
            log(self, "Failed to link reference type to target (node not found) " + l.referenceType(), LOG_LEVEL_ERROR)
          else:
            l.referenceType(tgt)
            referenceLinked = True
      else:
        referenceLinked = True

      if referenceLinked == True and targetLinked == True:
        linked.append(l)

    # References marked as "not forward" must be inverted (removed from source node, assigned to target node and relinked)
    log(self, "Inverting reference direction for all references with isForward==False attribute (is this correct!?)", LOG_LEVEL_WARN)
    for n in self.nodes:
      for r in n.getReferences():
        if r.isForward() == False:
          tgt = r.target()
          if isinstance(tgt, opcua_node_t):
            nref = opcua_referencePointer_t(n, parentNode=tgt)
            nref.referenceType(r.referenceType())
            tgt.addReference(nref)

    # Create inverse references for all nodes
    log(self, "Updating all referencedBy fields in nodes for inverse lookups.")
    for n in self.nodes:
      n.updateInverseReferences()

    for l in linked:
      self.__linkLater__.remove(l)

    if len(self.__linkLater__) != 0:
      log(self, str(len(self.__linkLater__)) + " could not be linked.", LOG_LEVEL_WARN)

  def sanitize(self):
    remove = []
    log(self, "Sanitizing nodes and references...")
    for n in self.nodes:
      if n.sanitize() == False:
        remove.append(n)
    if not len(remove) == 0:
      log(self, str(len(remove)) + " nodes will be removed because they failed sanitation.", LOG_LEVEL_WARN)
    # FIXME: Some variable ns=0 nodes fail because they don't have DataType fields...
    #        How should this be handles!?
    log(self, "Not actually removing nodes... it's unclear if this is valid or not", LOG_LEVEL_WARN)

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
    log(self, "Type definitions built/passed: " +  str(stat), LOG_LEVEL_DEBUG)

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

  def printOpen62541Header(self, printedExternally=[], supressGenerationOfAttribute=[], outfilename=""):
    unPrintedNodes = []
    unPrintedRefs  = []
    code = []
    header = []

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
        log(self, "Node " + str(n.id()) + " is being ignored.", LOG_LEVEL_DEBUG)
    for n in unPrintedNodes:
      for r in n.getReferences():
        if (r.target() != None) and (r.target().id() != None) and (r.parent() != None):
          unPrintedRefs.append(r)

    log(self, str(len(unPrintedNodes)) + " Nodes, " + str(len(unPrintedRefs)) +  "References need to get printed.", LOG_LEVEL_DEBUG)
    header.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")
    code.append("/* WARNING: This is a generated file.\n * Any manual changes will be overwritten.\n\n */")

    header.append("#ifndef "+outfilename.upper()+"_H_")
    header.append("#define "+outfilename.upper()+"_H_")

    header.append('#include "server/ua_server_internal.h"')
    header.append('#include "server/ua_nodes.h"')
    header.append('#include "ua_types.h"')
    header.append("extern void "+outfilename+"(UA_Server *server);\n")
    header.append("#endif /* "+outfilename.upper()+"_H_ */")

    code.append('#include "'+outfilename+'.h"')
    code.append("inline void "+outfilename+"(UA_Server *server) {")

    # Find all references necessary to create the namespace and
    # "Bootstrap" them so all other nodes can safely use these referencetypes whenever
    # they can locate both source and target of the reference.
    log(self, "Collecting all references used in the namespace.", LOG_LEVEL_DEBUG)
    refsUsed = []
    for n in self.nodes:
      for r in n.getReferences():
        if not r.referenceType() in refsUsed:
          refsUsed.append(r.referenceType())
    log(self, str(len(refsUsed)) + " reference types are used in the namespace, which will now get bootstrapped.", LOG_LEVEL_DEBUG)
    for r in refsUsed:
      code = code + r.printOpen62541CCode(unPrintedNodes, unPrintedRefs);

    # Note to self: do NOT - NOT! - try to iterate over unPrintedNodes!
    #               Nodes remove themselves from this list when printed.
    log(self, "Printing all other nodes.", LOG_LEVEL_DEBUG)
    for n in self.nodes:
      code = code + n.printOpen62541CCode(unPrintedNodes, unPrintedRefs, supressGenerationOfAttribute=supressGenerationOfAttribute)

    if len(unPrintedNodes) != 0:
      log(self, "" + str(len(unPrintedNodes)) + " nodes could not be translated to code.", LOG_LEVEL_WARN)
    else:
      log(self, "Printing suceeded for all nodes", LOG_LEVEL_DEBUG)

    if len(unPrintedRefs) != 0:
      log(self, "Attempting to print " + str(len(unPrintedRefs)) + " unprinted references.", LOG_LEVEL_DEBUG)
      tmprefs = []
      for r in unPrintedRefs:
        if  not (r.target() not in unPrintedNodes) and not (r.parent() in unPrintedNodes):
          if not isinstance(r.parent(), opcua_node_t):
            log(self, "Reference has no parent!", LOG_LEVEL_DEBUG)
          elif not isinstance(r.parent().id(), opcua_node_id_t):
            log(self, "Parents nodeid is not a nodeID!", LOG_LEVEL_DEBUG)
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
        log(self, "" + str(len(unPrintedRefs)) + " references could not be translated to code.", LOG_LEVEL_WARN)
    else:
      log(self, "Printing succeeded for all references", LOG_LEVEL_DEBUG)

    code.append("}")
    return (header,code)

###
### Testing
###

class testing:
  def __init__(self):
    self.namespace = opcua_namespace("testing")

    log(self, "Phase 1: Reading XML file nodessets")
    self.namespace.parseXML("Opc.Ua.NodeSet2.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.namespace.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    log(self, "Phase 2: Linking address space references and datatypes")
    self.namespace.linkOpenPointers()
    self.namespace.sanitize()

    log(self, "Phase 3: Comprehending DataType encoding rules")
    self.namespace.buildEncodingRules()

    log(self, "Phase 4: Allocating variable value data")
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
      print "Iteration: " + str(i)
      for n in ns:
        tmp.append(n)
        for r in n.getReferences():
          if (not r.target() in tmp):
           tmp.append(r.target())
      print "...tmp, " + str(len(tmp)) + " nodes discovered"
      ns = []
      for n in tmp:
        ns.append(n)
      print "...done, " + str(len(ns)) + " nodes discovered"

    log(self, "Phase 5: Printing pretty graph")
    self.namespace.printDotGraphWalk(depth=1, rootNode=self.namespace.getNodeByIDString("i=84"), followInverse=False, excludeNodeIds=["i=29", "i=22", "i=25"])
    #self.namespace.printDot()

class testing_open62541_header:
  def __init__(self):
    self.namespace = opcua_namespace("testing")

    log(self, "Phase 1: Reading XML file nodessets")
    self.namespace.parseXML("Opc.Ua.NodeSet2.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part4.xml")
    #self.namespace.parseXML("Opc.Ua.NodeSet2.Part5.xml")
    #self.namespace.parseXML("Opc.Ua.SimulationNodeSet2.xml")

    log(self, "Phase 2: Linking address space references and datatypes")
    self.namespace.linkOpenPointers()
    self.namespace.sanitize()

    log(self, "Phase 3: Calling C Printers")
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
