#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH


import logging
from datatypes import QualifiedName, LocalizedText, NodeId

__all__ = ['Reference', 'RefOrAlias', 'Node', 'ReferenceTypeNode',
           'ObjectNode', 'VariableNode', 'VariableTypeNode',
           'MethodNode', 'ObjectTypeNode', 'DataTypeNode', 'ViewNode']

logger = logging.getLogger(__name__)

class Reference:
    # all either nodeids or strings with an alias
    def __init__(self, source, referenceType, target, isForward):
        self.source = source
        self.referenceType = referenceType
        self.target = target
        self.isForward = isForward

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

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(str(self))

def RefOrAlias(s):
    try:
        return NodeId(s)
    except Exception:
        return s

class Node:
    def __init__(self):
        self.id = None
        self.browseName = None
        self.displayName = None
        self.description = None
        self.symbolicName = None
        self.writeMask = None
        self.userWriteMask = None
        self.references = dict() # using dict because it retains insertion order
        self.hidden = False
        self.modelUri = None
        self.parent = None
        self.parentReference = None
        self.namespaceMapping = {}

    def __str__(self):
        return self.__class__.__name__ + "(" + str(self.id) + ")"

    def __repr__(self):
        return str(self)

    def parseXML(self, xmlelement):
        for idname in ['NodeId', 'NodeID', 'nodeid']:
            if xmlelement.hasAttribute(idname):
                self.id = RefOrAlias(xmlelement.getAttribute(idname))

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
            elif at == "SymbolicName":
                self.symbolicName = av

        for x in xmlelement.childNodes:
            if x.nodeType != x.ELEMENT_NODE:
                continue
            if x.firstChild:
                if x.localName == "BrowseName":
                    self.browseName = QualifiedName(x.firstChild.data)
                elif x.localName == "DisplayName":
                    self.displayName = LocalizedText(x.firstChild.data)
                elif x.localName == "Description":
                    self.description = LocalizedText(x.firstChild.data)
                elif x.localName == "WriteMask":
                    self.writeMask = int(x.firstChild.data)
                elif x.localName == "UserWriteMask":
                    self.userWriteMask = int(x.firstChild.data)
                if x.localName == "References":
                    self.parseXMLReferences(x)

    def parseXMLReferences(self, xmlelement):
        for ref in xmlelement.childNodes:
            if ref.nodeType != ref.ELEMENT_NODE:
                continue
            source = RefOrAlias(str(self.id))  # deep-copy of the nodeid
            target = RefOrAlias(ref.firstChild.data)

            reftype = None
            forward = True
            for (at, av) in ref.attributes.items():
                if at == "ReferenceType":
                    reftype = RefOrAlias(av)
                elif at == "IsForward":
                    forward = "false" not in av.lower()
            self.references[Reference(source, reftype, target, forward)] = None

    def getParentReference(self, parentreftypes):
        # HasSubtype has precedence
        for ref in self.references:
            if ref.referenceType == NodeId("ns=0;i=45") and not ref.isForward:
                return ref
        for ref in self.references:
            if ref.referenceType in parentreftypes and not ref.isForward:
                return ref
        return None

    def popTypeDef(self):
        for ref in self.references:
            if ref.referenceType == NodeId("ns=0;i=40") and ref.isForward:
                del self.references[ref]
                return ref
        return Reference(NodeId(), NodeId(), NodeId(), False)

    def replaceAliases(self, aliases):
        if str(self.id) in aliases:
            self.id = NodeId(aliases[self.id])
        if isinstance(self, VariableNode) or isinstance(self, VariableTypeNode):
            if str(self.dataType) in aliases:
                self.dataType = NodeId(aliases[self.dataType])
        new_refs = dict()
        for ref in self.references:
            if str(ref.source) in aliases:
                ref.source = NodeId(aliases[ref.source])
            if str(ref.target) in aliases:
                ref.target = NodeId(aliases[ref.target])
            if str(ref.referenceType) in aliases:
                ref.referenceType = NodeId(aliases[ref.referenceType])
            new_refs[ref] = None
        self.references = new_refs

    def replaceNamespaces(self, nsMapping):
        self.id.ns = nsMapping[self.id.ns]
        self.browseName.ns = nsMapping[self.browseName.ns]
        if hasattr(self, 'dataType') and isinstance(self.dataType, NodeId):
            self.dataType.ns = nsMapping[self.dataType.ns]
        new_refs = dict()
        for ref in self.references:
            ref.source.ns = nsMapping[ref.source.ns]
            ref.target.ns = nsMapping[ref.target.ns]
            ref.referenceType.ns = nsMapping[ref.referenceType.ns]
            new_refs[ref] = None
        self.references = new_refs
        self.namespaceMapping = nsMapping

class ReferenceTypeNode(Node):
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.isAbstract = False
        self.symmetric = False
        self.inverseName = ""
        if xmlelement:
            ReferenceTypeNode.parseXML(self, xmlelement)

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
                if x.localName == "InverseName" and x.firstChild:
                    self.inverseName = str(x.firstChild.data)

class ObjectNode(Node):
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.eventNotifier = 0
        if xmlelement:
            ObjectNode.parseXML(self, xmlelement)

    def parseXML(self, xmlelement):
        Node.parseXML(self, xmlelement)
        for (at, av) in xmlelement.attributes.items():
            if at == "EventNotifier":
                self.eventNotifier = int(av)

class VariableNode(Node):
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.dataType = None
        self.valueRank = None
        self.arrayDimensions = []
        # Set access levels to read by default
        self.accessLevel = 1
        self.userAccessLevel = 1
        self.minimumSamplingInterval = 0.0
        self.historizing = False
        self.value = None
        if xmlelement:
            VariableNode.parseXML(self, xmlelement)

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
                self.dataType = RefOrAlias(av)
            elif  at == "ArrayDimensions":
                self.arrayDimensions = av.split(",")
            elif at == "Historizing":
                self.historizing = "false" not in av.lower()

        for x in xmlelement.childNodes:
            if x.nodeType != x.ELEMENT_NODE:
                continue
            if x.localName == "Value":
                self.value = x
            elif x.localName == "DataType":
                self.dataType = RefOrAlias(av)
            elif x.localName == "ValueRank":
                self.valueRank = int(x.firstChild.data)
            elif x.localName == "ArrayDimensions" and len(self.arrayDimensions) == 0:
                elements = x.getElementsByTagName("ListOfUInt32")
                if len(elements):
                    for _, v in enumerate(elements[0].getElementsByTagName("UInt32")):
                        self.arrayDimensions.append(v.firstChild.data)
            elif x.localName == "AccessLevel":
                self.accessLevel = int(x.firstChild.data)
            elif x.localName == "UserAccessLevel":
                self.userAccessLevel = int(x.firstChild.data)
            elif x.localName == "MinimumSamplingInterval":
                self.minimumSamplingInterval = float(x.firstChild.data)
            elif x.localName == "Historizing":
                self.historizing = "false" not in x.lower()

class VariableTypeNode(VariableNode):
    def __init__(self, xmlelement=None):
        VariableNode.__init__(self)
        self.isAbstract = False
        if xmlelement:
            VariableTypeNode.parseXML(self, xmlelement)

    def parseXML(self, xmlelement):
        VariableNode.parseXML(self, xmlelement)
        for (at, av) in xmlelement.attributes.items():
            if at == "IsAbstract":
                self.isAbstract = "false" not in av.lower()

        for x in xmlelement.childNodes:
            if x.nodeType != x.ELEMENT_NODE:
                continue
            if x.localName == "IsAbstract":
                self.isAbstract = "false" not in av.lower()

class MethodNode(Node):
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.executable = True
        self.userExecutable = True
        self.methodDecalaration = None
        if xmlelement:
            MethodNode.parseXML(self, xmlelement)

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
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.isAbstract = False
        if xmlelement:
            ObjectTypeNode.parseXML(self, xmlelement)

    def parseXML(self, xmlelement):
        Node.parseXML(self, xmlelement)
        for (at, av) in xmlelement.attributes.items():
            if at == "IsAbstract":
                self.isAbstract = "false" not in av.lower()

class DataTypeNode(Node):
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.isAbstract = False
        if xmlelement:
            DataTypeNode.parseXML(self, xmlelement)

    def parseXML(self, xmlelement):
        Node.parseXML(self, xmlelement)
        for (at, av) in xmlelement.attributes.items():
            if at == "IsAbstract":
                self.isAbstract = "false" not in av.lower()

class ViewNode(Node):
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.containsNoLoops = False
        self.eventNotifier = False
        if xmlelement:
            ViewNode.parseXML(self, xmlelement)

    def parseXML(self, xmlelement):
        Node.parseXML(self, xmlelement)
        for (at, av) in xmlelement.attributes.items():
            if at == "ContainsNoLoops":
                self.containsNoLoops = "false" not in av.lower()
            if at == "EventNotifier":
                self.eventNotifier = "false" not in av.lower()
