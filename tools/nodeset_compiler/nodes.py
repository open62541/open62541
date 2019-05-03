#!/usr/bin/env python
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
from datatypes import *

__all__ = ['Reference', 'RefOrAlias', 'Node', 'ReferenceTypeNode',
           'ObjectNode', 'VariableNode', 'VariableTypeNode',
           'MethodNode', 'ObjectTypeNode', 'DataTypeNode', 'ViewNode']

logger = logging.getLogger(__name__)

if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s

class Reference(object):
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

class Node(object):
    def __init__(self):
        self.id = None
        self.browseName = None
        self.displayName = None
        self.description = None
        self.symbolicName = None
        self.writeMask = None
        self.userWriteMask = None
        self.references = set()
        self.hidden = False
        self.modelUri = None
        self.parent = None
        self.parentReference = None

    def __str__(self):
        return self.__class__.__name__ + "(" + str(self.id) + ")"

    def __repr__(self):
        return str(self)

    def sanitize(self):
        pass

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
                self.symbolicName = String(av)

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
                    self.writeMask = int(unicode(x.firstChild.data))
                elif x.localName == "UserWriteMask":
                    self.userWriteMask = int(unicode(x.firstChild.data))
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
                    forward = not "false" in av.lower()
            self.references.add(Reference(source, reftype, target, forward))

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
            if ref.referenceType.i == 40 and ref.isForward:
                self.references.remove(ref)
                return ref
        return Reference(NodeId(), NodeId(), NodeId(), False)

    def replaceAliases(self, aliases):
        if str(self.id) in aliases:
            self.id = NodeId(aliases[self.id])
        if isinstance(self, VariableNode) or isinstance(self, VariableTypeNode):
            if str(self.dataType) in aliases:
                self.dataType = NodeId(aliases[self.dataType])
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

    def replaceNamespaces(self, nsMapping):
        self.id.ns = nsMapping[self.id.ns]
        self.browseName.ns = nsMapping[self.browseName.ns]
        if hasattr(self, 'dataType') and isinstance(self.dataType, NodeId):
            self.dataType.ns = nsMapping[self.dataType.ns]
        new_refs = set()
        for ref in self.references:
            ref.source.ns = nsMapping[ref.source.ns]
            ref.target.ns = nsMapping[ref.target.ns]
            ref.referenceType.ns = nsMapping[ref.referenceType.ns]
            new_refs.add(ref)
        self.references = new_refs

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
                    self.inverseName = str(unicode(x.firstChild.data))

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
        self.xmlValueDef = None
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

        for x in xmlelement.childNodes:
            if x.nodeType != x.ELEMENT_NODE:
                continue
            if x.localName == "Value":
                self.xmlValueDef = x
            elif x.localName == "DataType":
                self.dataType = RefOrAlias(av)
            elif x.localName == "ValueRank":
                self.valueRank = int(unicode(x.firstChild.data))
            elif x.localName == "ArrayDimensions" and len(self.arrayDimensions) == 0:
                elements = x.getElementsByTagName("ListOfUInt32");
                if len(elements):
                    for idx, v in enumerate(elements[0].getElementsByTagName("UInt32")):
                        self.arrayDimensions.append(v.firstChild.data)
            elif x.localName == "AccessLevel":
                self.accessLevel = int(unicode(x.firstChild.data))
            elif x.localName == "UserAccessLevel":
                self.userAccessLevel = int(unicode(x.firstChild.data))
            elif x.localName == "MinimumSamplingInterval":
                self.minimumSamplingInterval = float(unicode(x.firstChild.data))
            elif x.localName == "Historizing":
                self.historizing = "false" not in x.lower()

    def allocateValue(self, nodeset):
        dataTypeNode = nodeset.getDataTypeNode(self.dataType)
        if dataTypeNode is None:
            return False

        # FIXME: Don't build at all or allocate "defaults"? I'm for not building at all.
        if self.xmlValueDef is None:
            #logger.warn("Variable " + self.browseName() + "/" + str(self.id()) + " is not initialized. No memory will be allocated.")
            return False

        self.value = Value()
        self.value.parseXMLEncoding(self.xmlValueDef, dataTypeNode, self, nodeset.namespaceMapping[self.modelUri])
        return True


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
    """ DataTypeNode is a subtype of Node describing DataType nodes.

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
        1) A list of ("Fieldname", Node) tuples.
        2) A list of ("Fieldname", int) tuples.

        A DataType (and in consequence all Variables using it) shall be deemed not
        encodable if any of its fields cannot be traced to an encodable builtin type.

        A DataType shall be further deemed not encodable if it contains mixed structure/
        enumaration definitions.

        If encodable, the encoding can be retrieved using getEncoding().
    """

    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.isAbstract = False
        self.__xmlDefinition__ = None
        self.__baseTypeEncoding__ = []
        self.__encodable__ = None
        self.__definition__ = []
        self.__isEnum__     = False
        self.__isOptionSet__ = False
        if xmlelement:
            DataTypeNode.parseXML(self, xmlelement)

    def parseXML(self, xmlelement):
        Node.parseXML(self, xmlelement)
        for (at, av) in xmlelement.attributes.items():
            if at == "IsAbstract":
                self.isAbstract = "false" not in av.lower()

        for x in xmlelement.childNodes:
            if x.nodeType == x.ELEMENT_NODE:
                if x.localName == "Definition":
                    self.__xmlDefinition__ = x

    def isEncodable(self):
        """ Will return True if buildEncoding() was able to determine which builtin
            type corresponds to all fields of this DataType.

            If no encoding has been build yet an exception will be thrown.
            Make sure to call buildEncoding() first.
        """
        if self.__encodable__ is None:
            raise Exception("Encoding needs to be built first using buildEncoding()")
        return self.__encodable__

    def getEncoding(self):
        """ If the dataType is encodable, getEncoding() returns a nested list
            containing the encoding the structure definition for this type.

            If no encoding has been build yet an exception will be thrown.
            Make sure to call buildEncoding() first.

            If buildEncoding() has failed, an empty list will be returned.
        """
        if self.__encodable__ is None:
            raise Exception("Encoding needs to be built first using buildEncoding()")
        if not self.__encodable__:
            return []
        else:
            return self.__baseTypeEncoding__


    def buildEncoding(self, nodeset, indent=0, force=False):
        """ buildEncoding() determines the structure and aliases used for variables
            of this DataType.

            The function will parse the XML <Definition> of the dataType and extract
            "Name"-"Type" tuples. If successful, buildEncoding will return a nested
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

        prefix = " " + "|" * indent + "+"

        if force==True:
            self.__encodable__ = None

        if self.__encodable__ is not None and self.__encodable__:
            if self.isEncodable():
                logger.debug(prefix + str(self.__baseTypeEncoding__) + " (already analyzed)")
            else:
                logger.debug( prefix + str(self.__baseTypeEncoding__) + "(already analyzed, not encodable!)")
            return self.__baseTypeEncoding__

        self.__encodable__ = True

        if indent==0:
            logger.debug("Parsing DataType " + str(self.browseName) + " (" + str(self.id) + ")")

        if valueIsInternalType(self.browseName.name):
            self.__baseTypeEncoding__ = [self.browseName.name]
            self.__encodable__ = True
            logger.debug( prefix + str(self.browseName) + "*")
            logger.debug("Encodable as: " + str(self.__baseTypeEncoding__))
            logger.debug("")
            return self.__baseTypeEncoding__


        # Check if there is a supertype available
        parentType = None
        for ref in self.references:
            if ref.isForward:
                continue
                # hasSubtype
            if ref.referenceType.i == 45:
                targetNode = nodeset.nodes[ref.target]
                if targetNode is not None and isinstance(targetNode, DataTypeNode):
                    parentType = targetNode
                    break

        if self.__xmlDefinition__ is None:
            if parentType is not None:
                logger.debug( prefix + "Attempting definition using supertype " + str(targetNode.browseName) + " for DataType " + " " + str(self.browseName))
                subenc = targetNode.buildEncoding(nodeset=nodeset, indent=indent+1)
                if not targetNode.isEncodable():
                    self.__encodable__ = False
                else:
                    self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [self.browseName.name, subenc, None]
            if len(self.__baseTypeEncoding__) == 0:
                logger.debug(prefix + "No viable definition for " + str(self.browseName) + " " + str(self.id) + " found.")
                self.__encodable__ = False

            if indent==0:
                if not self.__encodable__:
                    logger.debug("Not encodable (partial): " + str(self.__baseTypeEncoding__))
                else:
                    logger.debug("Encodable as: " + str(self.__baseTypeEncoding__))
                logger.debug( "")

            return self.__baseTypeEncoding__

        isEnum = True
        isSubType = True
        # An option set is at the same time also an enum, at least for the encoding below
        isOptionSet = parentType is not None and parentType.id.ns == 0 and parentType.id.i==12755

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
                valueRank = None
                #symbolicName = None
                for at,av in x.attributes.items():
                    if at == "DataType":
                        fdtype = str(av)
                        if fdtype in nodeset.aliases:
                            fdtype = nodeset.aliases[fdtype]
                        isEnum = False
                    elif at == "Name":
                        fname = str(av)
                    #elif at == "SymbolicName":
                    #    symbolicName = str(av)
                    elif at == "Value":
                        enumVal = int(av)
                        isSubType = False
                    elif at == "ValueRank":
                        valueRank = int(av)
                    else:
                        logger.warn("Unknown Field Attribute " + str(at))
                # This can either be an enumeration OR a structure, not both.
                # Figure out which of the dictionaries gets the newly read value pair
                if isEnum == isSubType:
                    # This is an error
                    logger.warn("DataType contains both enumeration and subtype (or neither)")
                    self.__encodable__ = False
                    break
                elif isEnum:
                    # This is an enumeration
                    enumDict.append((fname, enumVal))
                    continue
                else:
                    if fdtype == "":
                        # If no datatype given use base datatype
                        fdtype = "i=24"

                    # This might be a subtype... follow the node defined as datatype to find out
                    # what encoding to use
                    fdTypeNodeId = NodeId(fdtype)
                    fdTypeNodeId.ns = nodeset.namespaceMapping[self.modelUri][fdTypeNodeId.ns]
                    if not fdTypeNodeId in nodeset.nodes:
                        raise Exception("Node {} not found in nodeset".format(fdTypeNodeId))
                    dtnode = nodeset.nodes[fdTypeNodeId]
                    # The node in the datatype element was found. we inherit its encoding,
                    # but must still ensure that the dtnode is itself validly encodable
                    typeDict.append([fname, dtnode])
                    fdtype = str(dtnode.browseName.name)
                    logger.debug( prefix + fname + " : " + fdtype + " -> " + str(dtnode.id))
                    subenc = dtnode.buildEncoding(nodeset=nodeset, indent=indent+1)
                    self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [[fname, subenc, valueRank]]
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

        if isOptionSet == True:
            self.__isOptionSet__ = True
            subenc = parentType.buildEncoding(nodeset=nodeset)
            if not parentType.isEncodable():
                self.__encodable__ = False
            else:
                self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [self.browseName.name, subenc, None]
                self.__definition__ = enumDict
            return self.__baseTypeEncoding__

        if isEnum == True:
            self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + ['Int32']
            self.__definition__ = enumDict
            self.__isEnum__ = True
            logger.debug( prefix+"Int32* -> enumeration with dictionary " + str(enumDict) + " encodable " + str(self.__encodable__))
            return self.__baseTypeEncoding__

        if indent==0:
            if not self.__encodable__:
                logger.debug( "Not encodable (partial): " + str(self.__baseTypeEncoding__))
            else:
                logger.debug( "Encodable as: " + str(self.__baseTypeEncoding__))
                self.__isEnum__ = False
                self.__definition__ = typeDict
            logger.debug( "")
        return self.__baseTypeEncoding__

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
