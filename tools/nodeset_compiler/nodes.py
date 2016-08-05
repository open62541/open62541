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

import sys
import logging
from sets import Set
from datatypes import *
from constants import *

logger = logging.getLogger(__name__)

if sys.version_info[0] >= 3:
  # strings are already parsed to unicode
  def unicode(s):
    return s

class Reference(object):
  # all either nodeids or strings with an alias
  def __init__(self, source, referenceType, target, isForward = True, hidden = False):
    self.source = source
    self.referenceType = referenceType
    self.target = target
    self.isForward = isForward
    self.hidden = hidden # the reference is part of a nodeset that already exists

  def __str__(self):
    retval = str(self.source)
    if not self.isForward:
      retval = retval + "<"
    retval = retval + "--[" + str(self.referenceType) + "]--"
    if self.isForward:
      retval = retval + ">"
    return retval + str(self.target)

  def __repr__(self):
      return str(self)

  def __eq__(self, other):
      return str(self) == str(other)

  def __hash__(self):
    return hash(str(self))

class Node(object):
  def __init__(self):
    self.id             = NodeId()
    self.nodeClass      = NODE_CLASS_GENERERIC
    self.browseName     = QualifiedName()
    self.displayName    = LocalizedText()
    self.description    = LocalizedText()
    self.writeMask      = 0
    self.userWriteMask  = 0
    self.references     = Set()
    self.inverseReferences = Set()
    self.hidden = False

  def __str__(self):
    return self.__class__.__name__ + "(" + str(self.id) + ")"

  def __repr__(self):
    return str(self)

  def sanitize(self):
    pass

  def parseXML(self, xmlelement):
    for idname in ['NodeId', 'NodeID', 'nodeid']:
      if xmlelement.hasAttribute(idname):
        self.id = NodeId(xmlelement.getAttribute(idname))

    for (at, av) in xmlelement.attributes.items():
      if at == "BrowseName":
        self.browseName = QualifiedName(av)
      elif at == "DisplayName":
        self.displayName = LocalizedText(av)
      elif at == "Description":
        self.description = LocalizedText(av)
      elif at == "WriteMask":
        self.writeMask = int(av)
      elif at == "UserWriteMask":
        self.userWriteMask = int(av)
      elif at == "EventNotifier":
        self.eventNotifier = int(av)

    for x in xmlelement.childNodes:
      if x.nodeType != x.ELEMENT_NODE:
        continue
      if x.firstChild:
        if x.tagName == "BrowseName":
          self.browseName = QualifiedName(x.firstChild.data)
        elif x.tagName == "DisplayName":
          self.displayName = LocalizedText(x.firstChild.data)
        elif x.tagName == "Description":
          self.description = LocalizedText(x.firstChild.data)
        elif x.tagName == "WriteMask":
          self.writeMask = int(unicode(x.firstChild.data))
        elif x.tagName == "UserWriteMask":
          self.userWriteMask = int(unicode(x.firstChild.data))
        if x.tagName == "References":
          self.parseXMLReferences(x)

  def parseXMLReferences(self, xmlelement):
    for ref in xmlelement.childNodes:
      if ref.nodeType != ref.ELEMENT_NODE:
        continue
      source = NodeId(str(self.id)) # deep-copy of the nodeid
      target = NodeId(ref.firstChild.data)
      reftype = None
      forward = True
      for (at, av) in ref.attributes.items():
        if at == "ReferenceType":
          if '=' in av:
            reftype = NodeId(av)
          else:
            reftype = av # alias, such as "HasSubType"
        elif at == "IsForward":
          forward = not "false" in av.lower()
      if forward:
        self.references.add(Reference(source, reftype, target, forward))
      else:
        self.inverseReferences.add(Reference(source, reftype, target, forward))

  def replaceAliases(self, aliases):
    if str(self.id) in aliases:
      self.id = NodeId(aliases[self.id])
    new_refs = set()
    for ref in self.references:
      if str(ref.source) in aliases:
        ref.source = NodeId(aliases[ref.source])
      if str(ref.target) in aliases:
        ref.target = NodeId(aliases[ref.target])
      if str(ref.referenceType) in aliases:
        ref.referenceType = NodeId(aliases[ref.referenceType])
      new_refs.add(ref)
    self.references = new_refs
    new_inv_refs = set()
    for ref in self.inverseReferences:
      if str(ref.source) in aliases:
        ref.source = NodeId(aliases[ref.source])
      if str(ref.target) in aliases:
        ref.target = NodeId(aliases[ref.target])
      if str(ref.referenceType) in aliases:
        ref.referenceType = NodeId(aliases[ref.referenceType])
      new_inv_refs.add(ref)
    self.inverseReferences = new_inv_refs

  def replaceNamespaces(self, nsMapping):
    self.id.ns = nsMapping[self.id.ns]
    self.browseName.ns = nsMapping[self.browseName.ns]
    new_refs = set()
    for ref in self.references:
      ref.source.ns = nsMapping[ref.source.ns]
      ref.target.ns = nsMapping[ref.target.ns]
      ref.referenceType.ns = nsMapping[ref.referenceType.ns]
      new_refs.add(ref)
    self.references = new_refs
    new_inv_refs = set()
    for ref in self.inverseReferences:
      ref.source.ns = nsMapping[ref.source.ns]
      ref.target.ns = nsMapping[ref.target.ns]
      ref.referenceType.ns = nsMapping[ref.referenceType.ns]
      new_inv_refs.add(ref)
    self.inverseReferences = new_inv_refs

class ReferenceTypeNode(Node):
  def __init__(self, xmlelement = None):
    Node.__init__(self)
    self.nodeClass = NODE_CLASS_REFERENCETYPE
    self.isAbstract    = False
    self.symmetric     = False
    self.inverseName   = ""
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    Node.parseXML(self, xmlelement)
    for (at, av) in xmlelement.attributes.items():
      if at == "Symmetric":
        self.symmetric = "false" not in av.lower()
      elif at == "InverseName":
        self.inverseName = str(av)
      elif at == "IsAbstract":
        self.isAbstract = "false" not in av.lower()

    for x in xmlelement.childNodes:
      if x.nodeType == x.ELEMENT_NODE:
        if x.tagName == "InverseName" and x.firstChild:
          self.inverseName = str(unicode(x.firstChild.data))

class ObjectNode(Node):
  def __init__(self, xmlelement = None):
    Node.__init__(self)
    self.nodeClass = NODE_CLASS_OBJECT
    self.eventNotifier = 0
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    Node.parseXML(self, xmlelement)
    for (at, av) in xmlelement.attributes.items():
      if at == "EventNotifier":
        self.eventNotifier = int(av)

class VariableNode(Node):
  def __init__(self, xmlelement = None):
    Node.__init__(self)
    self.nodeClass = NODE_CLASS_VARIABLE
    self.dataType            = NodeId()
    self.valueRank           = -1
    self.arrayDimensions     = []
    self.accessLevel         = 0
    self.userAccessLevel     = 0
    self.minimumSamplingInterval = 0.0
    self.historizing         = False
    self.value               = None
    self.xmlValueDef         = None
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    Node.parseXML(self, xmlelement)
    for (at, av) in xmlelement.attributes.items():
      if at == "ValueRank":
        self.valueRank = int(av)
      elif at == "AccessLevel":
        self.accessLevel = int(av)
      elif at == "UserAccessLevel":
        self.userAccessLevel = int(av)
      elif at == "MinimumSamplingInterval":
        self.minimumSamplingInterval = float(av)
      elif at == "DataType":
        if "=" in av:
          self.dataType = NodeId(av)
        else:
          self.dataType = av

    for x in xmlelement.childNodes:
      if x.nodeType != x.ELEMENT_NODE:
        continue
      if x.tagName == "Value":
          self.__xmlValueDef__ = x
      elif x.tagName == "DataType":
          self.dataType = NodeId(str(x))
      elif x.tagName == "ValueRank":
          self.valueRank = int(unicode(x.firstChild.data))
      elif x.tagName == "ArrayDimensions":
          self.arrayDimensions = int(unicode(x.firstChild.data))
      elif x.tagName == "AccessLevel":
          self.accessLevel = int(unicode(x.firstChild.data))
      elif x.tagName == "UserAccessLevel":
          self.userAccessLevel = int(unicode(x.firstChild.data))
      elif x.tagName == "MinimumSamplingInterval":
          self.minimumSamplingInterval = float(unicode(x.firstChild.data))
      elif x.tagName == "Historizing":
          self.historizing = "false" not in x.lower()

class VariableTypeNode(VariableNode):
  def __init__(self, xmlelement = None):
    VariableNode.__init__(self)
    self.nodeClass = NODE_CLASS_VARIABLETYPE
    if xmlelement:
      self.parseXML(xmlelement)

class MethodNode(Node):
  def __init__(self, xmlelement = None):
    Node.__init__(self)
    self.nodeClass = NODE_CLASS_METHOD
    self.executable     = True
    self.userExecutable = True
    self.methodDecalaration = None
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    Node.parseXML(self, xmlelement)
    for (at, av) in xmlelement.attributes.items():
      if at == "Executable":
        self.executable = "false" not in av.lower()
      if at == "UserExecutable":
        self.userExecutable = "false" not in av.lower()
      if at == "MethodDeclarationId":
        self.methodDeclaration = str(av)

class ObjectTypeNode(Node):
  def __init__(self, xmlelement = None):
    Node.__init__(self)
    self.nodeClass = NODE_CLASS_OBJECTTYPE
    self.isAbstract = False
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    Node.parseXML(self, xmlelement)
    for (at, av) in xmlelement.attributes.items():
      if at == "IsAbstract":
        self.isAbstract = "false" not in av.lower()

class DataTypeNode(Node):
  def __init__(self, xmlelement = None):
    Node.__init__(self)
    self.nodeClass = NODE_CLASS_DATATYPE
    self.isAbstract = False
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    Node.parseXML(self, xmlelement)
    for (at, av) in xmlelement.attributes.items():
      if at == "IsAbstract":
        self.isAbstract = "false" not in av.lower()

class ViewNode(Node):
  def __init__(self, xmlelement = None):
    Node.__init__(self)
    self.nodeClass = NODE_CLASS_VIEW
    self.containsNoLoops == False
    self.eventNotifier == False
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    Node.parseXML(self, xmlelement)
    for (at, av) in xmlelement.attributes.items():
      if at == "ContainsNoLoops":
        self.containsNoLoops = "false" not in av.lower()
      if at == "eventNotifier":
        self.eventNotifier = "false" not in av.lower()
