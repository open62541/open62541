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

from logger import *;
from ua_builtin_types import *;
from open62541_MacroHelper import open62541_MacroHelper
from ua_constants import *

def getNextElementNode(xmlvalue):
  if xmlvalue == None:
    return None
  xmlvalue = xmlvalue.nextSibling
  while not xmlvalue == None and not xmlvalue.nodeType == xmlvalue.ELEMENT_NODE:
    xmlvalue = xmlvalue.nextSibling
  return xmlvalue

###
### References are not really described by OPC-UA. This is how we
### use them here.
###

class opcua_referencePointer_t():
  """ Representation of a pointer.

      A pointer consists of a target (which should be a node class),
      an optional reference type (which should be an instance of
      opcua_node_referenceType_t) and an optional isForward flag.
  """
  __reference_type__          = None
  __target__                  = None
  __isForward__               = True
  __addr__                    = 0
  __parentNode__              = None

  def __init__(self, target, hidden=False, parentNode=None):
    self.__target__ = target
    self.__reference_type__ = None
    self.__isForward__ = True
    self.__isHidden__ = hidden
    self.__parentNode__ = parentNode
    self.__addr__ = 0

  def isHidden(self, data=None):
    if isinstance(data, bool):
      self.__isHidden__ = data
    return self.__isHidden__

  def isForward(self, data=None):
    if isinstance(data, bool):
      self.__isForward__ = data
    return self.__isForward__

  def referenceType(self, type=None):
    if not type == None:
      self.__reference_type__ = type
    return self.__reference_type__

  def target(self, data=None):
    if not data == None:
      self.__target__ = data
    return self.__target__

  def address(self, data=None):
    if data != None:
      self.__addr__ = data
    return self.__addr__

  def parent(self):
    return self.__parentNode__

  def getCodePrintableID(self):
    src = "None"
    tgt = "None"
    type = "Unknown"
    if self.parent() != None:
      src = str(self.parent().id())
    if self.target() != None:
      tgt = str(self.target().id())
    if self.referenceType() != None:
      type = str(self.referenceType().id())
    tmp = src+"_"+type+"_"+tgt
    tmp = tmp.lower()
    refid = ""
    for i in tmp:
      if not i in "ABCDEFGHIJKLMOPQRSTUVWXYZ0123456789".lower():
        refid = refid + ("_")
      else:
        refid = refid + i
    return refid

  def __str__(self):
    retval=""
    if isinstance(self.parent(), opcua_node_t):
      if isinstance(self.parent().id(), opcua_node_id_t):
        retval=retval + str(self.parent().id()) + "--["
      else:
        retval=retval + "(?) --["
    else:
      retval=retval + "(?) --["

    if isinstance(self.referenceType(), opcua_node_t):
      retval=retval + str(self.referenceType().browseName()) + "]-->"
    else:
      retval=retval + "?]-->"

    if isinstance(self.target(), opcua_node_t):
      if isinstance(self.target().id(), opcua_node_id_t):
        retval=retval + str(self.target().id())
      else:
        retval=retval + "(?) "
    else:
      retval=retval + "(?) "

    if self.isForward() or self.isHidden():
      retval = retval + " <"
      if self.isForward():
        retval = retval + "F"
      if self.isHidden():
        retval = retval + "H"
      retval = retval + ">"
    return retval

  def __repr__(self):
      return self.__str__()

  def __cmp__(self, other):
    if not isinstance(other, opcua_referencePointer_t):
      return -1
    if other.target() == self.target():
      if other.referenceType() == self.referenceType():
        return 0
    return 1


###
### Node ID's as a builtin type are useless. using this one instead.
###

class opcua_node_id_t():
  """ Implementation of a node ID.

      The ID will encoding itself appropriatly as string. If multiple ID's (numeric, string, guid)
      are defined, the order of preference for the ID string is always numeric, guid,
      bytestring, string. Binary encoding only applies to numeric values (UInt16).
  """
  i = -1
  o = ""
  g = ""
  s = ""
  ns = 0
  __mystrname__ = ""

  def __init__(self, idstring):
    idparts = idstring.split(";")
    self.i = None
    self.b = None
    self.g = None
    self.s = None
    self.ns = 0
    for p in idparts:
      if p[:2] == "ns":
        self.ns = int(p[3:])
      elif p[:2] == "i=":
        self.i = int(p[2:])
      elif p[:2] == "o=":
        self.b = p[2:]
      elif p[:2] == "g=":
        tmp = []
        self.g = p[2:].split("-")
        for i in self.g:
          i = "0x"+i
          tmp.append(int(i,16))
        self.g = tmp
      elif p[:2] == "s=":
        self.s = p[2:]
    self.__mystrname__ = ""
    self.toString()

  def toString(self):
    self.__mystrname__ = ""
    if self.ns != 0:
      self.__mystrname__ = "ns="+str(self.ns)+";"
    # Order of preference is numeric, guid, bytestring, string
    if self.i != None:
      self.__mystrname__ = self.__mystrname__ + "i="+str(self.i)
    elif self.g != None:
      self.__mystrname__ = self.__mystrname__ + "g="
      tmp = []
      for i in self.g:
        tmp.append(hex(i).replace("0x",""))
      for i in tmp:
        self.__mystrname__ = self.__mystrname__ + "-" + i
      self.__mystrname__ = self.__mystrname__.replace("g=-","g=")
    elif self.b != None:
      self.__mystrname__ = self.__mystrname__ + "b="+str(self.b)
    elif self.s != None:
      self.__mystrname__ = self.__mystrname__ + "s="+str(self.s)

  def __str__(self):
    return self.__mystrname__

  def __repr__(self):
    return self.__mystrname__

###
### Actually existing node types
###

class opcua_node_t:
  __node_id__             = None
  __node_class__          = 0
  __node_browseName__     = ""
  __node_displayName__    = ""
  __node_description__    = ""
  __node_writeMask__      = 0
  __node_userWriteMask__  = 0
  __node_namespace__      = None
  __node_references__     = []
  __node_referencedBy__   = []
  __binary__              = ""
  __address__             = 0

  def __init__(self, id, ns):
    self.__node_namespace__      = ns
    self.__node_id__             = id
    self.__node_class__          = 0
    self.__node_browseName__     = ""
    self.__node_displayName__    = ""
    self.__node_description__    = ""
    self.__node_writeMask__      = 0
    self.__node_userWriteMask__  = 0
    self.__node_references__     = []
    self.__node_referencedBy__   = []
    self.__init_subType__()
    self.FLAG_ISABSTRACT       = 128
    self.FLAG_SYMMETRIC        = 64
    self.FLAG_CONTAINSNOLOOPS  = 32
    self.FLAG_EXECUTABLE       = 16
    self.FLAG_USEREXECUTABLE   = 8
    self.FLAG_HISTORIZING      = 4
    self.__binary__            = ""

  def __init_subType__(self):
    self.nodeClass(0)

  def __str__(self):
    if isinstance(self.id(), opcua_node_id_t):
      return self.__class__.__name__ + "(" + str(self.id()) + ")"
    return self.__class__.__name__ + "( no ID )"

  def __repr__(self):
    if isinstance(self.id(), opcua_node_id_t):
      return self.__class__.__name__ + "(" + str(self.id()) + ")"
    return self.__class__.__name__ + "( no ID )"


  def getCodePrintableID(self):
    CodePrintable="NODE_"

    if isinstance(self.id(), opcua_node_id_t):
      CodePrintable = self.__class__.__name__ + "_" +  str(self.id())
    else:
      CodePrintable = self.__class__.__name__ + "_unknown_nid"

    CodePrintable = CodePrintable.lower()
    cleanPrintable = ""
    for i in range(0,len(CodePrintable)):
      if not CodePrintable[i] in "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_".lower():
        cleanPrintable = cleanPrintable + "_"
      else:
        cleanPrintable = cleanPrintable + CodePrintable[i]
    return cleanPrintable

  def addReference(self, ref):
    """ Add a opcua_referencePointer_t to the list of
        references this node carries.
    """
    if not ref in self.__node_references__:
      self.__node_references__.append(ref)

  def removeReference(self, ref):
    if ref in self.__node_references__:
      self.__node_references__.remove(ref)

  def removeReferenceToNode(self, targetNode):
    tmp = []
    if ref in self.__node_references__:
      if ref.target() != targetNode:
        tmp.append(ref)
    self.__node_references__ = tmp

  def addInverseReferenceTarget(self, node):
    """ Adds a reference to the inverse reference list of this node.

        Inverse references are considered as "this node is referenced by"
        and facilitate lookups when between nodes that reference this node,
        but are not referenced by this node. These references would
        require the namespace to be traversed by references to be found
        if this node was not aware of them.
    """
    # Only add this target if it is not already referenced
    if not node in self.__node_referencedBy__:
      if not self.hasReferenceTarget(node):
        self.__node_referencedBy__.append(opcua_referencePointer_t(node, hidden=True, parentNode=self))
#        log(self, self.__node_browseName__ + " added reverse reference to " + str(node.__node_browseName__), LOG_LEVEL_DEBUG)
#      else:
#        log(self, self.__node_browseName__ + " refusing reverse reference to " + str(node.__node_browseName__)  + " (referenced normally)", LOG_LEVEL_DEBUG)
#    else:
#      log(self, self.__node_browseName__ + " refusing reverse reference to " + str(node.__node_browseName__) + " (already reversed referenced)", LOG_LEVEL_DEBUG)

  def getReferences(self):
    return self.__node_references__

  def getInverseReferences(self):
    return self.__node_referencedBy__

  def hasInverseReferenceTarget(self, node):
    for r in self.getInverseReferences():
      if node == r.target():
        return True
    return False

  def hasReferenceTarget(self, node):
    for r in self.getReferences():
      if node == r.target():
        return True
    return False

  def getFirstParentNode(self):
    """ getFirstParentNode

        return a tuple of (opcua_node_t, opcua_referencePointer_t) indicating
        the first node found that references this node. If this node is not
        referenced at all, None will be returned.

        This function requires a linked namespace.

        Note that there may be more than one nodes that reference this node.
        The parent returned will be determined by the first isInverse()
        Reference of this node found. If none exists, the first hidden
        reference will be returned.
    """
    parent = None
    revref = None

    for hiddenstatus in [False, True]:
      for r in self.getReferences():
        if r.isHidden() == hiddenstatus and r.isForward() == False:
          parent = r.target()
          for r in parent.getReferences():
            if r.target() == self:
              revref = r
              break
          if revref != None:
            return (parent, revref)

    return (parent, revref)

  def updateInverseReferences(self):
    """ Updates inverse references in all nodes referenced by this node.

        The function will look up all referenced nodes and check if they
        have a reference that points back at this node. If none is found,
        that means that the target is not aware that this node references
        it. In that case an inverse reference will be registered with
        the target node to point back to this node instance.
    """
    # Update inverse references in all nodes we have referenced
    for r in self.getReferences():
      if isinstance(r.target(), opcua_node_t):
        if not r.target().hasInverseReferenceTarget(self):
          #log(self, self.__node_browseName__ + " req. rev. referencing in" + str(r.target().__node_browseName__), LOG_LEVEL_DEBUG)
          r.target().addInverseReferenceTarget(self)
      #else:
        #log(self, "Cannot register inverse link to " + str(r.target()) + " (not a node)")

  def id(self):
    return self.__node_id__

  def getNamespace(self):
    return self.__node_namespace__

  def nodeClass(self, c = 0):
    """ Sets the node class attribute if c is passed.
        Returns the current node class.
    """
    # Allow overwriting only if it is not set
    if isinstance(c, int):
      if self.__node_class__ == 0 and c < 256:
        self.__node_class__ = c
    return self.__node_class__

  def browseName(self, data=0):
    """ Sets the browse name attribute if data is passed.
        Returns the current browse name.
    """
    if isinstance(data, str):
      self.__node_browseName__ = data
    return self.__node_browseName__.encode('utf-8')

  def displayName(self, data=None):
    """ Sets the display name attribute if data is passed.
        Returns the current display name.
    """
    if data != None:
      self.__node_displayName__ = data
    return self.__node_displayName__.encode('utf-8')

  def description(self, data=None):
    """ Sets the description attribute if data is passed.
        Returns the current description.
    """
    if data != None:
      self.__node_description__ = data
    return self.__node_description__.encode('utf-8')

  def writeMask(self, data=None):
    """ Sets the write mask attribute if data is passed.
        Returns the current write mask.
    """
    if data != None:
      self.__node_writeMask__ = data
    return self.__node_writeMask__

  def userWriteMask(self, data=None):
    """ Sets the user write mask attribute if data is passed.
        Returns the current user write mask.
    """
    if data != None:
      self.__node_userWriteMask__ = data
    return self.__node_userWriteMask__

  def initiateDummyXMLReferences(self, xmlelement):
    """ Initiates references found in the XML <References> element.

        All references initiated will be registered with this node, but
        their targets will be strings extracted from the XML description
        (hence "dummy").

        References created will however be registered with the namespace
        for linkLater(), which will eventually replace the string target
        with an actual instance of an opcua_node_t.
    """
    if not xmlelement.tagName == "References":
      log(self, "XMLElement passed is not a reference list", LOG_LEVEL_ERROR)
      return
    for ref in xmlelement.childNodes:
      if ref.nodeType == ref.ELEMENT_NODE:
        dummy = opcua_referencePointer_t(unicode(ref.firstChild.data), parentNode=self)
        self.addReference(dummy)
        self.getNamespace().linkLater(dummy)
        for (at, av) in ref.attributes.items():
          if at == "ReferenceType":
            dummy.referenceType(av)
          elif at == "IsForward":
            if "false" in av.lower():
              dummy.isForward(False)
          else:
            log(self, "Don't know how to process attribute " + at + "(" + av + ") for references.", LOG_LEVEL_ERROR)

  def printDot(self):
    cleanname = "node_" + str(self.id()).replace(";","").replace("=","")
    dot = cleanname + " [label = \"{" + str(self.id()) + "|" + str(self.browseName()) + "}\", shape=\"record\"]"
    for r in self.__node_references__:
      if isinstance(r.target(), opcua_node_t):
        tgtname = "node_" + str(r.target().id()).replace(";","").replace("=","")
        dot = dot + "\n"
        if r.isForward() == True:
          dot = dot + cleanname + " -> " + tgtname + " [label=\"" + str(r.referenceType().browseName()) + "\"]\n"
        else:
          if len(r.referenceType().inverseName()) == 0:
            log(self, "Inverse name of reference is null " + str(r.referenceType().id()), LOG_LEVEL_WARN)
          dot = dot + cleanname + " -> " + tgtname + " [label=\"" + str(r.referenceType().inverseName()) + "\"]\n"
    return dot

  def sanitize(self):
    """ Check the health of this node.

        Return True if all manditory attributes are valid and all references have been
        correclty linked to nodes. Returns False on failure, which should indicate
        that this node needs to be removed from the namespace.
    """
    # Do we have an id?
    if not isinstance(self.id(), opcua_node_id_t):
      log(self, "HELP! I'm an id'less node!", LOG_LEVEL_ERROR)
      return False

    # Remove unlinked references
    tmp = []
    for r in self.getReferences():
      if not isinstance(r, opcua_referencePointer_t):
        log(self, "Reference is not a reference!?.", LOG_LEVEL_ERROR)
      elif not isinstance(r.referenceType(), opcua_node_t):
        log(self, "Reference has no valid reference type and will be removed.", LOG_LEVEL_ERROR)
      elif not isinstance(r.target(), opcua_node_t):
        log(self, "Reference to " + str(r.target()) + " is not a node. It has been removed.", LOG_LEVEL_WARN)
      else:
        tmp.append(r)
    self.__node_references__ = tmp

    # Make sure that every inverse referenced node actually does reference us
    tmp = []
    for r in self.getInverseReferences():
      if not isinstance(r.target(), opcua_node_t):
        log(self, "Invers reference to " + str(r.target()) + " does not reference a real node. It has been removed.", LOG_LEVEL_WARN)
      else:
        if r.target().hasReferenceTarget(self):
          tmp.append(r)
        else:
          log(self, "Node " + str(self.id()) + " was falsely under the impression that it is referenced by " + str(r.target().id()), LOG_LEVEL_WARN)
    self.__node_referencedBy__ = tmp

    # Remove references from inverse list if we can reach this not "the regular way"
    # over a normal reference
    tmp=[]
    for r in self.getInverseReferences():
      if not self.hasReferenceTarget(r.target()):
        tmp.append(r)
      else:
        log(self, "Removing unnecessary inverse reference to " + str(r.target.id()))
    self.__node_referencedBy__ = tmp

    return self.sanitizeSubType()

  def sanitizeSubType(self):
    pass

  def address(self, addr = None):
    """ If addr is passed, the address of this node within the binary
        representation will be set.

        If an address within the binary representation is known/set, this
        function will return it. NoneType is returned if no address has been
        set.
    """
    if addr != None:
      self.__address__ = addr
    return self.__address__

  def parseXML(self, xmlelement):
    """ Extracts base attributes from the XML description of an element.
        Parsed basetype attributes are:
        * browseName
        * displayName
        * Description
        * writeMask
        * userWriteMask
        * eventNotifier

        ParentNodeIds are ignored.

        If recognized, attributes and elements found will be removed from
        the XML Element passed. Type-specific attributes and child elements
        are handled by the parseXMLSubType() functions of all classes deriving
        from this base type and will be called automatically.
    """
    thisxml = xmlelement
    for (at, av) in thisxml.attributes.items():
      if at == "NodeId":
        xmlelement.removeAttribute(at)
      elif at == "BrowseName":
        self.browseName(str(av))
        xmlelement.removeAttribute(at)
      elif at == "DisplayName":
        self.displayName(av)
        xmlelement.removeAttribute(at)
      elif at == "Description":
        self.description(av)
        xmlelement.removeAttribute(at)
      elif at == "WriteMask":
        self.writeMask(int(av))
        xmlelement.removeAttribute(at)
      elif at == "UserWriteMask":
        self.userWriteMask(int(av))
        xmlelement.removeAttribute(at)
      elif at == "EventNotifier":
        self.eventNotifier(int(av))
        xmlelement.removeAttribute(at)
      elif at == "ParentNodeId":
        # Silently ignore this one..
        xmlelement.removeAttribute(at)

    for x in thisxml.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        if   x.tagName == "BrowseName":
          self.browseName(unicode(x.firstChild.data))
          xmlelement.removeChild(x)
        elif x.tagName == "DisplayName":
          self.displayName(unicode(x.firstChild.data))
          xmlelement.removeChild(x)
        elif x.tagName == "Description":
          self.description(unicode(x.firstChild.data))
          xmlelement.removeChild(x)
        elif x.tagName == "WriteMask":
          self.writeMask(int(unicode(x.firstChild.data)))
          xmlelement.removeChild(x)
        elif x.tagName == "UserWriteMask":
          self.userWriteMask(int(unicode(x.firstChild.data)))
          xmlelement.removeChild(x)
        elif x.tagName == "References":
          self.initiateDummyXMLReferences(x)
          xmlelement.removeChild(x)
    self.parseXMLSubType(xmlelement)

  def parseXMLSubType(self, xmlelement):
    pass

  def printXML(self):
    pass

  def printOpen62541CCode_Subtype(self):
    """ printOpen62541CCode_Subtype

        Specific node subtype does it's own initialization
    """
    return []

  def printOpen62541CCode(self, unPrintedNodes=[], unPrintedReferences=[], supressGenerationOfAttribute=[]):
    """ printOpen62541CCode

        Returns a list of strings containing the C-code necessary to intialize
        this node for the open62541 OPC-UA Stack.

        Note that this function will fail if the nodeid is non-numeric, as
        there is no UA_EXPANDEDNNODEID_[STRING|GUID|BYTESTRING] macro.
    """
    codegen = open62541_MacroHelper(supressGenerationOfAttribute=supressGenerationOfAttribute)
    code = []
    code.append("")
    code.append("do {")

    # Just to be sure...
    if not (self in unPrintedNodes):
      log(self, str(self) + " attempted to reprint already printed node " + str(self)+ ".", LOG_LEVEL_WARN)
      return []
    code = code + codegen.getCreateNode(self)
    code = code + self.printOpen62541CCode_Subtype()

    # If we are being passed a parent node by the namespace, use that for
    # Registering ourselves in the namespace
    parent = self.getFirstParentNode()
    if not (parent[0] in unPrintedNodes) and (parent[0] != None):
      if parent[1].referenceType() != None:
        code.append("// Referencing node found and declared as parent: " + str(parent[0].id()) + "/" + str(parent[0].__node_browseName__) + " using " + str(parent[1].referenceType().id()) + "/" + str(parent[1].referenceType().__node_browseName__))
        code.append("UA_Server_addNode(server, (UA_Node*) " + self.getCodePrintableID() + ", " + codegen.getCreateExpandedNodeIDMacro(parent[0]) + ", " + codegen.getCreateNodeIDMacro(parent[1].referenceType()) + ");")
    
    # Otherwise use the "Bootstrapping" method and we will get registered
    # with other nodes later.
    else:
      code.append("// Parent node does not exist yet. This node will be bootstrapped and linked later.")
      code.append("UA_NodeStore_insert(server->nodestore, (UA_Node*) " + self.getCodePrintableID() + ", UA_NULL);")
    # Note: getFirstParentNode will return [parentNode, referenceToChild]
    (parentNode, parentRef) = self.getFirstParentNode()
    if not (parentNode in unPrintedNodes) and (parentNode != None):
      if parentRef.referenceType() != None:
        code.append("// Referencing node found and declared as parent: " + str(parentNode .id()) + "/" + str(parentNode .__node_browseName__) + " using " + str(parentRef.referenceType().id()) + "/" + str(parentRef.referenceType().__node_browseName__))
        code.append("UA_Server_addNode(server, (UA_Node*) " + self.getCodePrintableID() + ", " + codegen.getCreateExpandedNodeIDMacro(parentNode ) + ", " + codegen.getCreateNodeIDMacro(parentRef.referenceType()) + ");")
        # Parent to child reference is added by the server, do not reprint that reference
        if parentRef in unPrintedReferences:
          unPrintedReferences.remove(parentRef)
    # Try to print all references to nodes that already exist
    # Note: we know the reference types exist, because the namespace class made sure they were
    #       the first ones being printed
    tmprefs = []
    for r in self.getReferences():
      #log(self, "Checking if reference from " + str(r.parent()) + "can be created...", LOG_LEVEL_DEBUG)
      if not (r.target() in unPrintedNodes):
        if r in unPrintedReferences:
          if (len(tmprefs) == 0):
            code.append("// This node has the following references that can be created:")
          code = code + codegen.getCreateStandaloneReference(self, r)
          tmprefs.append(r)
    # Remove printed refs from list
    for r in tmprefs:
      unPrintedReferences.remove(r)

    # Again, but this time check if other nodes deffered their node creation because this node did
    # not exist...
    tmprefs = []
    for r in unPrintedReferences:
      #log(self, "Checking if another reference " + str(r.target()) + "can be created...", LOG_LEVEL_DEBUG)
      if (r.target() == self) and not (r.parent() in unPrintedNodes):
        if not isinstance(r.parent(), opcua_node_t):
          log(self, "Reference has no parent!", LOG_LEVEL_DEBUG)
        elif not isinstance(r.parent().id(), opcua_node_id_t):
          log(self, "Parents nodeid is not a nodeID!", LOG_LEVEL_DEBUG)
        else:
          if (len(tmprefs) == 0):
            code.append("//  Creating this node has resolved the following open references:")
          code = code + codegen.getCreateStandaloneReference(r.parent(), r)
          tmprefs.append(r)
    # Remove printed refs from list
    for r in tmprefs:
      unPrintedReferences.remove(r)

    # Again, just to be sure...
    if self in unPrintedNodes:
      # This is necessery to make printing work at all!
      unPrintedNodes.remove(self)
    
    code.append("} while(0);")
    return code

class opcua_node_referenceType_t(opcua_node_t):
  __isAbstract__    = False
  __symmetric__     = False
  __reference_inverseName__   = ""
  __reference_referenceType__ = None

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_REFERENCETYPE)
    self.__reference_isAbstract__    = False
    self.__reference_symmetric__     = False
    self.__reference_inverseName__   = ""
    self.__reference_referenceType__ = None

  def referenceType(self,data=None):
    if isinstance(data, opcua_node_t):
      self.__reference_referenceType__ = data
    return self.__reference_referenceType__

  def isAbstract(self,data=None):
    if isinstance(data, bool):
      self.__isAbstract__ = data
    return self.__isAbstract__

  def symmetric(self,data=None):
    if isinstance(data, bool):
      self.__symmetric__ = data
    return self.__symmetric__

  def inverseName(self,data=None):
    if isinstance(data, str):
      self.__reference_inverseName__ = data
    return self.__reference_inverseName__

  def sanitizeSubType(self):
    if not isinstance(self.referenceType(), opcua_referencePointer_t):
      log(self, "ReferenceType " + str(self.referenceType()) + " of " + str(self.id()) + " is not a pointer (ReferenceType is manditory for references).", LOG_LEVEL_ERROR)
      self.__reference_referenceType__ = None
      return False
    return True

  def parseXMLSubType(self, xmlelement):
    for (at, av) in xmlelement.attributes.items():
      if at == "Symmetric":
        if "false" in av.lower():
          self.symmetric(False)
        else:
          self.symmetric(True)
        xmlelement.removeAttribute(at)
      elif at == "InverseName":
        self.inverseName(str(av))
        xmlelement.removeAttribute(at)
      elif at == "IsAbstract":
        if "false" in str(av).lower():
          self.isAbstract(False)
        else:
          self.isAbstract(True)
        xmlelement.removeAttribute(at)
      else:
        log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_ERROR)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        if x.tagName == "InverseName":
          self.inverseName(str(unicode(x.firstChild.data)))
        else:
          log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_INFO)

  def printOpen62541CCode_Subtype(self):
    code = []
    # Note that UA_FALSE is default here
    if self.isAbstract():
      code.append(self.getCodePrintableID() + "->isAbstract = UA_TRUE;")

    if self.symmetric():
      code.append(self.getCodePrintableID() + "->symmetric  = UA_TRUE;")

    if self.__reference_inverseName__ != "":
      code.append(self.getCodePrintableID() + "->inverseName  = UA_LOCALIZEDTEXT_ALLOC(\"en_US\", \"" + self.__reference_inverseName__ + "\");")
    return code;


class opcua_node_object_t(opcua_node_t):
  __object_eventNotifier__ = 0

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_OBJECT)
    self.__object_eventNotifier__ = 0

  def eventNotifier(self, data=""):
    if isinstance(data, int):
      self.__object_eventNotifier__ == data
    return self.__object_eventNotifier__

  def parseXMLSubType(self, xmlelement):
    for (at, av) in xmlelement.attributes.items():
      if at == "EventNotifier":
        self.eventNotifier(int(av))
        xmlelement.removeAttribute(at)
      elif at == "SymbolicName":
        # Silently ignore this one
        xmlelement.removeAttribute(at)
      else:
        log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_ERROR)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_INFO)

  def printOpen62541CCode_Subtype(self):
    code = []

    code.append(self.getCodePrintableID() + "->eventNotifier = (UA_Byte) " + str(self.eventNotifier()) + ";")
    return code

class opcua_node_variable_t(opcua_node_t):
  __value__               = 0
  __dataType__            = None
  __valueRank__           = 0
  __arrayDimensions__     = 0
  __accessLevel__         = 0
  __userAccessLevel__     = 0
  __minimumSamplingInterval__ = 0
  __historizing__         = False

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_VARIABLE)
    self.__value__               = None
    self.__dataType__            = None
    self.__valueRank__           = -1
    self.__arrayDimensions__     = []
    self.__accessLevel__         = 0
    self.__userAccessLevel__     = 0
    self.__minimumSamplingInterval__ = 0.0
    self.__historizing__         = False
    self.__xmlValueDef__         = None

  def value(self, data=0):
    if isinstance(data, opcua_value_t):
      self.__value__ = data
    return self.__value__

  def dataType(self, data=None):
    if data != None:
      self.__dataType__ = data
    return self.__dataType__

  def valueRank(self, data=""):
    if isinstance(data, int):
      self.__valueRank__ = data
    return self.__valueRank__

  def arrayDimensions(self, data=None):
    if not data==None:
      self.__arrayDimensions__ = data
    return self.__arrayDimensions__

  def accessLevel(self, data=None):
    if  not data==None:
      self.__accessLevel__ = data
    return self.__accessLevel__

  def userAccessLevel(self, data=None):
    if  not data==None:
      self.__userAccessLevel__ = data
    return self.__userAccessLevel__

  def minimumSamplingInterval(self, data=None):
    if  not data==None:
      self.__minimumSamplingInterval__ = data
    return self.__minimumSamplingInterval__

  def historizing(self, data=None):
    if data != None:
      self.__historizing__ = data
    return self.__historizing__

  def sanitizeSubType(self):
    if not isinstance(self.dataType(), opcua_referencePointer_t):
      log(self, "DataType " + str(self.dataType()) + " of " + str(self.id()) + " is not a pointer (DataType is manditory for variables).", LOG_LEVEL_ERROR)
      self.__dataType__ = None
      return False

    if not isinstance(self.dataType().target(), opcua_node_t):
      log(self, "DataType " + str(self.dataType().target()) + " of " + str(self.id()) + " does not point to a node (DataType is manditory for variables).", LOG_LEVEL_ERROR)
      self.__dataType__ = None
      return False
    return True

  def allocateValue(self):
    if not isinstance(self.dataType(), opcua_referencePointer_t):
      log(self, "Variable " + self.browseName() + "/" + str(self.id()) + " does not reference a valid dataType.", LOG_LEVEL_ERROR)
      return False

    if not isinstance(self.dataType().target(), opcua_node_dataType_t):
      log(self, "Variable " + self.browseName() + "/" + str(self.id()) + " does not have a valid dataType reference.", LOG_LEVEL_ERROR)
      return False

    if not self.dataType().target().isEncodable():
      log(self, "DataType for Variable " + self.browseName() + "/" + str(self.id()) + " is not encodable.", LOG_LEVEL_ERROR)
      return False

    # FIXME: Don't build at all or allocate "defaults"? I'm for not building at all.
    if self.__xmlValueDef__ == None:
      #log(self, "Variable " + self.browseName() + "/" + str(self.id()) + " is not initialized. No memory will be allocated.", LOG_LEVEL_WARN)
      return False

    self.value(opcua_value_t(self))
    self.value().parseXML(self.__xmlValueDef__)

    # Array Dimensions must accurately represent the value and will be patched
    # reflect the exaxt dimensions attached binary stream.
    if not isinstance(self.value(), opcua_value_t) or len(self.value().value) == 0:
      self.arrayDimensions([])
    else:
      # Parser only permits 1-d arrays, which means we do not have to check further dimensions
      self.arrayDimensions([len(self.value().value)])
    return True

  def parseXMLSubType(self, xmlelement):
    for (at, av) in xmlelement.attributes.items():
      if at == "ValueRank":
        self.valueRank(int(av))
        xmlelement.removeAttribute(at)
      elif at == "AccessLevel":
        self.accessLevel(int(av))
        xmlelement.removeAttribute(at)
      elif at == "UserAccessLevel":
        self.userAccessLevel(int(av))
        xmlelement.removeAttribute(at)
      elif at == "MinimumSamplingInterval":
        self.minimumSamplingInterval(float(av))
        xmlelement.removeAttribute(at)
      elif at == "DataType":
        self.dataType(opcua_referencePointer_t(str(av), parentNode=self))
        # dataType needs to be linked to a node once the namespace is read
        self.getNamespace().linkLater(self.dataType())
        xmlelement.removeAttribute(at)
      elif at == "SymbolicName":
        # Silently ignore this one
        xmlelement.removeAttribute(at)
      else:
        log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_ERROR)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        if x.tagName == "Value":
          # We need to be able to parse the DataType to build the variable value,
          # which can only be done if the namespace is linked.
          # Store the Value for later parsing
          self.__xmlValueDef__ = x
          #log(self,  "Value description stored for later elaboration.", LOG_LEVEL_DEBUG)
        elif x.tagName == "DataType":
          self.dataType(opcua_referencePointer_t(str(av), parentNode=self))
          # dataType needs to be linked to a node once the namespace is read
          self.getNamespace().linkLater(self.dataType())
        elif x.tagName == "ValueRank":
          self.valueRank(int(unicode(x.firstChild.data)))
        elif x.tagName == "ArrayDimensions":
          self.arrayDimensions(int(unicode(x.firstChild.data)))
        elif x.tagName == "AccessLevel":
          self.accessLevel(int(unicode(x.firstChild.data)))
        elif x.tagName == "UserAccessLevel":
          self.userAccessLevel(int(unicode(x.firstChild.data)))
        elif x.tagName == "MinimumSamplingInterval":
          self.minimumSamplingInterval(float(unicode(x.firstChild.data)))
        elif x.tagName == "Historizing":
          if "true" in x.firstChild.data.lower():
            self.historizing(True)
        else:
          log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_INFO)

  def printOpen62541CCode_Subtype(self):
    code = []
    codegen = open62541_MacroHelper()

    if self.historizing():
      code.append(self.getCodePrintableID() + "->historizing = UA_TRUE;")

    code.append(self.getCodePrintableID() + "->minimumSamplingInterval = (UA_Double) " + str(self.minimumSamplingInterval()) + ";")
    code.append(self.getCodePrintableID() + "->userAccessLevel = (UA_Int32) " + str(self.userAccessLevel()) + ";")
    code.append(self.getCodePrintableID() + "->accessLevel = (UA_Int32) " + str(self.accessLevel()) + ";")
    code.append(self.getCodePrintableID() + "->valueRank = (UA_Int32) " + str(self.valueRank()) + ";")

    if self.dataType() != None and isinstance(self.dataType().target(), opcua_node_dataType_t):
      # Delegate the encoding of the datavalue to the helper if we have
      # determined a valid encoding
      if self.dataType().target().isEncodable():
        if self.value() != None:
          code = code + self.value().printOpen62541CCode()
    return code

class opcua_node_method_t(opcua_node_t):
  __executable__     = True
  __userExecutable__ = True
  __methodDecalaration__ = None

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_METHOD)
    self.__executable__     = True
    self.__userExecutable__ = True
    self.__methodDecalaration__ = None

  def methodDeclaration(self, data=None):
    if not data==None:
      self.__methodDecalaration__ = data
    return self.__methodDecalaration__

  def executable(self, data=None):
    if isinstance(data, bool):
      self.__executable__ == data
    return self.__executable__

  def userExecutable(self, data=None):
    if isinstance(data, bool):
      self.__userExecutable__ == data
    return self.__userExecutable__

  def sanitizeSubType(self):
    if self.methodDeclaration() != None:
      if not isinstance(self.methodDeclaration().target(), opcua_node_t):
        return False
    else:
      #FIXME: Is this even permitted!?
      pass

  def parseXMLSubType(self, xmlelement):
    for (at, av) in xmlelement.attributes.items():
      if at == "MethodDeclarationId":
        self.methodDeclaration(opcua_referencePointer_t(str(av), parentNode=self))
        # dataType needs to be linked to a node once the namespace is read
        self.getNamespace().linkLater(self.methodDeclaration())
      else:
        log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_ERROR)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_INFO)

  def printOpen62541CCode_Subtype(self):
    code = []

    # UA_False is default for booleans on _init()
    if self.executable():
      code.append(self.getCodePrintableID() + "->executable = UA_TRUE;")
    if self.userExecutable():
      code.append(self.getCodePrintableID() + "->userExecutable = UA_TRUE;")

    return code

class opcua_node_objectType_t(opcua_node_t):
  __isAbstract__ = False

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_OBJECTTYPE)
    self.__isAbstract__ == False

  def isAbstract(self, data=None):
    if isinstance(data, bool):
      self.__isAbstract__ = data
    return self.__isAbstract__

  def parseXMLSubType(self, xmlelement):
    for (at, av) in xmlelement.attributes.items():
      if at == "IsAbstract":
        if "false" in av.lower():
          self.isAbstract(False)
        xmlelement.removeAttribute(at)
      else:
        log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_ERROR)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_INFO)

  def printOpen62541CCode_Subtype(self):
    code = []

    if (self.isAbstract()):
      code.append(self.getCodePrintableID() + "->isAbstract = UA_TRUE;")
    #else:
    #  code.append(self.getCodePrintableID() + "->isAbstract = UA_FALSE;")

    return code

class opcua_node_variableType_t(opcua_node_t):
  __value__ = 0
  __dataType__ = None
  __valueRank__ = 0
  __arrayDimensions__ = 0
  __isAbstract__ = False
  __xmlDefinition__ = None

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_VARIABLETYPE)
    self.__value__ = 0
    self.__dataType__ = None
    self.__valueRank__ = -1
    self.__arrayDimensions__ = 0
    self.__isAbstract__ = False
    self.__xmlDefinition__ = None

  def value(self, data=None):
    log(self, "Setting data not implemented!", LOG_LEVEL_ERROR)

  def dataType(self, data=None):
    if data != None:
      self.__dataType__ = data
    return self.__dataType__

  def valueRank(self,data=None):
    if isinstance(data, int):
      self.__valueRank__ = data
    return self.__valueRank__

  def arrayDimensions(self,data=None):
    if isinstance(data, int):
      self.__arrayDimensions__ = data
    return self.__arrayDimensions__

  def isAbstract(self,data=None):
    if isinstance(data, bool):
      self.__isAbstract__ = data
    return self.__isAbstract__

  def sanitizeSubType(self):
    # DataType fields appear to be optional for VariableTypes
    # but if it does have a node set, it must obviously be a valid node
    if not self.dataType() != None:
      if not isinstance(self.dataType(), opcua_referencePointer_t):
        log(self, "DataType attribute of " + str(self.id()) + " is not a pointer", LOG_LEVEL_ERROR)
        return False
      else:
        if not isinstance(self.dataType().target(), opcua_node_t):
          log(self, "DataType attribute of " + str(self.id()) + " does not point to a node", LOG_LEVEL_ERROR)
          return False
    else:
      # FIXME: It's unclear wether this is ok or not.
      log(self, "DataType attribute of variableType " + str(self.id()) + " is not defined.", LOG_LEVEL_WARN)
      return False

  def parseXMLSubType(self, xmlelement):
    for (at, av) in xmlelement.attributes.items():
      if at == "IsAbstract":
        if "false" in av.lower():
          self.isAbstract(False)
        else:
          self.isAbstract(True)
        xmlelement.removeAttribute(at)
      elif at == "ValueRank":
        self.valueRank(int(av))
        if self.valueRank() != -1:
          log(self, "Array's or matrices are only permitted in variables and not supported for variableTypes. This attribute will effectively be ignored.", LOG_LEVEL_WARN)
        xmlelement.removeAttribute(at)
      elif at == "DataType":
        self.dataType(opcua_referencePointer_t(str(av), parentNode=self))
        # dataType needs to be linked to a node once the namespace is read
        self.getNamespace().linkLater(self.dataType())
      else:
        log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_ERROR)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        if x.tagName == "Definition":
          self.__xmlDefinition__ = x
          log(self,  "Definition stored for future processing", LOG_LEVEL_DEBUG)
        else:
          log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_INFO)

  def printOpen62541CCode_Subtype(self):
    code = []

    if (self.isAbstract()):
      code.append(self.getCodePrintableID() + "->isAbstract = UA_TRUE;")
    else:
      code.append(self.getCodePrintableID() + "->isAbstract = UA_FALSE;")
    return code

class opcua_node_dataType_t(opcua_node_t):
  """ opcua_node_dataType_t is a subtype of opcua_note_t describing DataType nodes.

      DataType contain definitions and structure information usable for Variables.
      The format of this structure is determined by buildEncoding()
      Two definition styles are distinguished in XML:
      1) A DataType can be a structure of fields, each field having a name and a type.
         The type must be either an encodable builtin node (ex. UInt32) or point to
         another DataType node that inherits its encoding from a builtin type using
         a inverse "hasSubtype" (hasSuperType) reference.
      2) A DataType may be an enumeration, in which each field has a name and a numeric
         value.
      The definition is stored as an ordered list of tuples. Depending on which
      definition style was used, the __definition__ will hold
      1) A list of ("Fieldname", opcua_node_t) tuples.
      2) A list of ("Fieldname", int) tuples.

      A DataType (and in consequence all Variables using it) shall be deemed not
      encodable if any of its fields cannot be traced to an encodable builtin type.

      A DataType shall be further deemed not encodable if it contains mixed structure/
      enumaration definitions.

      If encodable, the encoding can be retrieved using getEncoding().
  """
  __isAbstract__ = False
  __isEnum__     = False
  __xmlDefinition__ = None
  __baseTypeEncoding__ = []
  __encodable__ = False
  __encodingBuilt__ = False
  __definition__ = []

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_DATATYPE)
    self.__isAbstract__ == False
    self.__xmlDefinition__ = None
    self.__baseTypeEncoding__ = []
    self.__encodable__ = None
    self.__encodingBuilt__ = False
    self.__definition__ = []
    self.__isEnum__     = False

  def isAbstract(self,data=None):
    """ Will return True if isAbstract was defined.

        Calling this function with an arbitrary data parameter will set
        isAbstract = data.
    """
    if isinstance(data, bool):
      self.__isAbstract__ = data
    return self.__isAbstract__

  def isEncodable(self):
    """ Will return True if buildEncoding() was able to determine which builtin
        type corresponds to all fields of this DataType.

        If no encoding has been build yet, this function will call buildEncoding()
        and return True if it succeeds.
    """
    return self.__encodable__

  def getEncoding(self):
    """ If the dataType is encodable, getEncoding() returns a nested list
        containing the encoding the structure definition for this type.

        If no encoding has been build yet, this function will call buildEncoding()
        and return the encoding if buildEncoding() succeeds.

        If buildEncoding() fails or has failed, an empty list will be returned.
    """
    if self.__encodable__ == False:
      if self.__encodingBuilt__ == False:
        return self.buildEncoding()
      return []
    else:
      return self.__baseTypeEncoding__

  def buildEncoding(self, indent=0, force=False):
    """ buildEncoding() determines the structure and aliases used for variables
        of this DataType.

        The function will parse the XML <Definition> of the dataType and extract
        "Name"-"Type" tuples. If successfull, buildEncoding will return a nested
        list of the following format:

        [['Alias1', ['Alias2', ['BuiltinType']]], [Alias2, ['BuiltinType']], ...]

        Aliases are fieldnames defined by this DataType or DataTypes referenced. A
        list such as ['DataPoint', ['Int32']] indicates that a value will encode
        an Int32 with the alias 'DataPoint' such as <DataPoint>12827</DataPoint>.
        Only the first Alias of a nested list is considered valid for the BuiltinType.

        Single-Elemented lists are always BuiltinTypes. Every nested list must
        converge in a builtin type to be encodable. buildEncoding will follow
        the first type inheritance reference (hasSupertype) of the dataType if
        necessary;

        If instead to "DataType" a numeric "Value" attribute is encountered,
        the DataType will be considered an enumeration and all Variables using
        it will be encoded as Int32.

        DataTypes can be either structures or enumeration - mixed definitions will
        be unencodable.

        Calls to getEncoding() will be iterative. buildEncoding() can be called
        only once per dataType, with all following calls returning the predetermined
        value. Use of the 'force=True' parameter will force the Definition to be
        reparsed.

        After parsing, __definition__ holds the field definition as a list. Note
        that this might deviate from the encoding, especially if inheritance was
        used.
    """

    proxy = opcua_value_t(None)
    prefix = " " + "|"*indent+ "+"

    if force==True:
      self.__encodingBuilt__ = False

    if self.__encodingBuilt__ == True:
      if self.isEncodable():
        log(self, prefix + str(self.__baseTypeEncoding__) + " (already analyzed)")
      else:
        log(self,  prefix + str(self.__baseTypeEncoding__) + "(already analyzed, not encodable!)")
      return self.__baseTypeEncoding__
    self.__encodingBuilt__ = True # signify that we have attempted to built this type
    self.__encodable__ = True

    if indent==0:
      log(self, "Parsing DataType " + self.browseName() + " (" + str(self.id()) + ")")

    if proxy.isBuiltinByString(self.browseName()):
      self.__baseTypeEncoding__ = [self.browseName()]
      self.__encodable__ = True
      log(self,  prefix + self.browseName() + "*")
      log(self, "Encodable as: " + str(self.__baseTypeEncoding__))
      log(self, "")
      return self.__baseTypeEncoding__

    if self.__xmlDefinition__ == None:
      # Check if there is a supertype available
      for ref in self.getReferences():
        if "hassubtype" in ref.referenceType().browseName().lower() and ref.isForward() == False:
          if isinstance(ref.target(), opcua_node_dataType_t):
            log(self,  prefix + "Attempting definition using supertype " + ref.target().browseName() + " for DataType " + " " + self.browseName())
            subenc = ref.target().buildEncoding(indent=indent+1)
            if not ref.target().isEncodable():
              self.__encodable__ = False
              break
            else:
              self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [self.browseName(), subenc, 0]
      if len(self.__baseTypeEncoding__) == 0:
        log(self, prefix + "No viable definition for " + self.browseName() + " " + str(self.id()) + " found.")
        self.__encodable__ = False

      if indent==0:
        if not self.__encodable__:
          log(self, "Not encodable (partial): " + str(self.__baseTypeEncoding__))
        else:
          log(self, "Encodable as: " + str(self.__baseTypeEncoding__))
        log(self,  "")

      return self.__baseTypeEncoding__

    isEnum = True
    isSubType = True
    hasValueRank = 0

    # We need to store the definition as ordered data, but can't use orderedDict
    # for backward compatibility with Python 2.6 and 3.4
    enumDict = []
    typeDict = []

    # An XML Definition is provided and will be parsed... now
    for x in self.__xmlDefinition__.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        fname  = ""
        fdtype = ""
        enumVal = ""
        hasValueRank = 0
        for at,av in x.attributes.items():
          if at == "DataType":
            fdtype = str(av)
            isEnum = False
          elif at == "Name":
            fname = str(av)
          elif at == "Value":
            enumVal = int(av)
            isSubType = False
          elif at == "ValueRank":
            hasValueRank = int(av)
            log(self, "Arrays or matrices (ValueRank) are not supported for datatypes. This DT will become scalar.", LOG_LEVEL_WARN)
          else:
            log(self, "Unknown Field Attribute " + str(at), LOG_LEVEL_WARN)
        # This can either be an enumeration OR a structure, not both.
        # Figure out which of the dictionaries gets the newly read value pair
        if isEnum == isSubType:
          # This is an error
          log(self, "DataType contains both enumeration and subtype (or neither)", LOG_LEVEL_WARN)
          self.__encodable__ = False
          break
        elif isEnum:
          # This is an enumeration
          enumDict.append((fname, enumVal))
          continue
        else:
          # This might be a subtype... follow the node defined as datatype to find out
          # what encoding to use
          dtnode = self.getNamespace().getNodeByIDString(fdtype)
          if dtnode == None:
            # Node found in datatype element is invalid
            log(self,  prefix + fname + " ?? " + av + " ??")
            self.__encodable__ = False
          else:
            # The node in the datatype element was found. we inherit its encoding,
            # but must still ensure that the dtnode is itself validly encodable
            typeDict.append([fname, dtnode])
            if hasValueRank < 0:
              hasValueRank = 0
            fdtype = str(dtnode.browseName()) + "+"*hasValueRank
            log(self,  prefix + fname + " : " + fdtype + " -> " + str(dtnode.id()))
            subenc = dtnode.buildEncoding(indent=indent+1)
            self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [[fname, subenc, hasValueRank]]
            if not dtnode.isEncodable():
              # If we inherit an encoding from an unencodable not, this node is
              # also not encodable
              self.__encodable__ = False
              break

    # If we used inheritance to determine an encoding without alias, there is a
    # the possibility that lists got double-nested despite of only one element
    # being encoded, such as [['Int32']] or [['alias',['int32']]]. Remove that
    # enclosing list.
    while len(self.__baseTypeEncoding__) == 1 and isinstance(self.__baseTypeEncoding__[0], list):
      self.__baseTypeEncoding__ = self.__baseTypeEncoding__[0]

    if isEnum == True:
      self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + ['Int32']
      self.__definition__ = enumDict
      self.__isEnum__ = True
      log(self,  prefix+"Int32* -> enumeration with dictionary " + str(enumDict) + " encodable " + str(self.__encodable__))
      return self.__baseTypeEncoding__

    if indent==0:
      if not self.__encodable__:
        log(self,  "Not encodable (partial): " + str(self.__baseTypeEncoding__))
      else:
        log(self,  "Encodable as: " + str(self.__baseTypeEncoding__))
        self.__isEnum__ = False
        self.__definition__ = typeDict
      log(self,  "")
    return self.__baseTypeEncoding__

  def parseXMLSubType(self, xmlelement):
    """ Parses all XML data that is not considered part of the base node attributes.

        XML attributes fields processed are "isAbstract"
        XML elements processed are "Definition"
    """
    for (at, av) in xmlelement.attributes.items():
      if at == "IsAbstract":
        if "true" in str(av).lower():
          self.isAbstract(True)
        else:
          self.isAbstract(False)
        xmlelement.removeAttribute(at)
      else:
        log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_WARN)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        if x.tagName == "Definition":
          self.__xmlDefinition__ = x
          #log(self,  "Definition stored for future processing", LOG_LEVEL_DEBUG)
        else:
          log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_WARN)

  def encodedTypeId(self):
    """ Returns a number of the builtin Type that should be used
        to represent this datatype.
    """
    if self.isEncodable() != True or len(self.getEncoding()) == 0:
      # Encoding is []
      return 0
    else:
      enc = self.getEncoding()
      if len(enc) > 1 and isinstance(enc[0], list):
        # [ [?], [?], [?] ]
        # Encoding is a list representing an extensionobject
        return opcua_BuiltinType_extensionObject_t(None).getNumericRepresentation()
      else:
        if len(enc)==1 and isinstance(enc[0], str):
          # [ 'BuiltinType' ]
          return opcua_value_t(None).getTypeByString(enc[0]).getNumericRepresentation()
        else:
          # [ ['Alias', [?]] ]
          # Determine if [?] is reducable to a builtin type or if [?] is an aliased
          # extensionobject
          while len(enc) > 1 and isinstance(enc[0], str):
            enc = enc[1]
          if len(enc) > 1:
            return opcua_BuiltinType_extensionObject_t(None).getNumericRepresentation()
          else:
            return opcua_value_t(None).getTypeByString(enc[0]).getNumericRepresentation()

  def printOpen62541CCode_Subtype(self):
    code = []

    if (self.isAbstract()):
      code.append(self.getCodePrintableID() + "->isAbstract = UA_TRUE;")
    else:
      code.append(self.getCodePrintableID() + "->isAbstract = UA_FALSE;")
    return code

class opcua_node_view_t(opcua_node_t):
  __containsNoLoops__ = True
  __eventNotifier__   = 0

  def __init_subType__(self):
    self.nodeClass(NODE_CLASS_VIEW)
    self.__containsNoLoops__ == False
    self.__eventNotifier__ == False

  def containsNoLoops(self,data=None):
    if isinstance(data, bool):
      self.__containsNoLoops__ = data
    return self.__containsNoLoops__

  def eventNotifier(self,data=None):
    if isinstance(data, int):
      self.__eventNotifier__ = data
    return self.__eventNotifier__

  def parseXMLSubtype(self, xmlelement):
    for (at, av) in xmlelement.attributes.items():
      log(self, "Don't know how to process attribute " + at + " (" + av + ")", LOG_LEVEL_ERROR)

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        log(self,  "Unprocessable XML Element: " + x.tagName, LOG_LEVEL_INFO)

  def printOpen62541CCode_Subtype(self):
    code = []

    if self.containsNoLoops():
      code.append(self.getCodePrintableID() + "->containsNoLoops = UA_TRUE;")
    else:
      code.append(self.getCodePrintableID() + "->containsNoLoops = UA_FALSE;")

    code.append(self.getCodePrintableID() + "->eventNotifier = (UA_Byte) " + str(self.eventNotifier()) + ";")

    return code
