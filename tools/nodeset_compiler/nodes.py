#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Author:  Chris Iatrou (ichrispa@core-vector.net)
"""

import sys
import logging
from datatypes import *

__all__ = ['Reference', 'ref_or_alias', 'Node', 'ReferenceTypeNode',
           'ObjectNode', 'VariableNode', 'VariableTypeNode',
           'MethodNode', 'ObjectTypeNode', 'DataTypeNode', 'ViewNode']

logger = logging.getLogger(__name__)

if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s


class Reference(object):
    # all either nodeids or strings with an alias
    def __init__(self, source, reference_type, target, is_forward):
        self.source = source
        self.referenceType = reference_type
        self.target = target
        self.isForward = is_forward

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


def ref_or_alias(s):
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
        self.eventNotifier = None
        self.references = set()
        self.hidden = False
        self.modelUri = None
        self.parent = None
        self.parentReference = None
        self.dataType = None

    def __str__(self):
        return self.__class__.__name__ + "(" + str(self.id) + ")"

    def __repr__(self):
        return str(self)

    @staticmethod
    def sanitize():
        return True

    def parse_xml(self, xml_element):
        for id_name in ['NodeId', 'NodeID', 'nodeid']:
            if xml_element.hasAttribute(id_name):
                self.id = ref_or_alias(xml_element.getAttribute(id_name))

        for (at, av) in xml_element.attributes.items():
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

        for x in xml_element.childNodes:
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
                    self.parse_xml_references(x)

    def parse_xml_references(self, xml_element):
        for ref in xml_element.childNodes:
            if ref.nodeType != ref.ELEMENT_NODE:
                continue
            source = ref_or_alias(str(self.id))  # deep-copy of the nodeid
            target = ref_or_alias(ref.firstChild.data)

            ref_type = None
            forward = True
            for (at, av) in ref.attributes.items():
                if at == "ReferenceType":
                    ref_type = ref_or_alias(av)
                elif at == "IsForward":
                    forward = "false" not in av.lower()
            self.references.add(Reference(source, ref_type, target, forward))

    def get_parent_reference(self, parent_ref_types):
        # HasSubtype has precedence
        for ref in self.references:
            if ref.referenceType == NodeId("ns=0;i=45") and not ref.isForward:
                return ref
        for ref in self.references:
            if ref.referenceType in parent_ref_types and not ref.isForward:
                return ref
        return None

    def pop_type_def(self):
        for ref in self.references:
            if ref.referenceType.i == 40 and ref.isForward:
                self.references.remove(ref)
                return ref
        return Reference(NodeId(), NodeId(), NodeId(), False)

    def replace_aliases(self, aliases):
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

    def replace_namespaces(self, ns_mapping):
        self.id.ns = ns_mapping[self.id.ns]
        self.browseName.ns = ns_mapping[self.browseName.ns]
        if hasattr(self, 'dataType') and isinstance(self.dataType, NodeId):
            self.dataType.ns = ns_mapping[self.dataType.ns]
        new_refs = set()
        for ref in self.references:
            ref.source.ns = ns_mapping[ref.source.ns]
            ref.target.ns = ns_mapping[ref.target.ns]
            ref.referenceType.ns = ns_mapping[ref.referenceType.ns]
            new_refs.add(ref)
        self.references = new_refs


class ReferenceTypeNode(Node):
    def __init__(self, xml_element=None):
        Node.__init__(self)
        self.isAbstract = False
        self.symmetric = False
        self.inverseName = ""
        if xml_element:
            ReferenceTypeNode.parse_xml(self, xml_element)

    def parse_xml(self, xml_element):
        Node.parse_xml(self, xml_element)
        for (at, av) in xml_element.attributes.items():
            if at == "Symmetric":
                self.symmetric = "false" not in av.lower()
            elif at == "InverseName":
                self.inverseName = str(av)
            elif at == "IsAbstract":
                self.isAbstract = "false" not in av.lower()

        for x in xml_element.childNodes:
            if x.nodeType == x.ELEMENT_NODE:
                if x.localName == "InverseName" and x.firstChild:
                    self.inverseName = str(unicode(x.firstChild.data))


class ObjectNode(Node):
    def __init__(self, xml_element=None):
        Node.__init__(self)
        self.eventNotifier = 0
        if xml_element:
            ObjectNode.parse_xml(self, xml_element)

    def parse_xml(self, xml_element):
        Node.parse_xml(self, xml_element)
        for (at, av) in xml_element.attributes.items():
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
            VariableNode.parse_xml(self, xmlelement)

    def parse_xml(self, xml_element):
        Node.parse_xml(self, xml_element)
        attribute_type, attribute_value = None, None
        for (attribute_type, attribute_value) in xml_element.attributes.items():
            if attribute_type == "ValueRank":
                self.valueRank = int(attribute_value)
            elif attribute_type == "AccessLevel":
                self.accessLevel = int(attribute_value)
            elif attribute_type == "UserAccessLevel":
                self.userAccessLevel = int(attribute_value)
            elif attribute_type == "MinimumSamplingInterval":
                self.minimumSamplingInterval = float(attribute_value)
            elif attribute_type == "DataType":
                self.dataType = ref_or_alias(attribute_value)
            elif attribute_type == "ArrayDimensions":
                self.arrayDimensions = attribute_value.split(",")

        for x in xml_element.childNodes:
            if x.nodeType != x.ELEMENT_NODE:
                continue
            if x.localName == "Value":
                self.xmlValueDef = x
            elif x.localName == "DataType":
                self.dataType = ref_or_alias(attribute_value)
            elif x.localName == "ValueRank":
                self.valueRank = int(unicode(x.firstChild.data))
            elif x.localName == "ArrayDimensions" and len(self.arrayDimensions) == 0:
                elements = x.getElementsByTagName("ListOfUInt32")
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

    def allocate_value(self, nodeset):
        data_type_node = nodeset.get_data_type_node(self.dataType)
        if data_type_node is None:
            return False

        # FIXME: Don't build at all or allocate "defaults"? I'm for not building at all.
        if self.xmlValueDef is None:
            # logger.warn("Variable " + self.browseName() + "/" + str(self.id()) +
            #             " is not initialized. No memory will be allocated.")
            return False

        self.value = Value()
        self.value.parse_xml_encoding(self.xmlValueDef, data_type_node,
                                      self, nodeset.namespace_mapping[self.modelUri])
        return True


class VariableTypeNode(VariableNode):
    def __init__(self, xml_element=None):
        VariableNode.__init__(self)
        self.isAbstract = False
        if xml_element:
            VariableTypeNode.parse_xml(self, xml_element)

    def parse_xml(self, xml_element):
        VariableNode.parse_xml(self, xml_element)
        attribute_type, attribute_value = None, None
        for (attribute_type, attribute_value) in xml_element.attributes.items():
            if attribute_type == "IsAbstract":
                self.isAbstract = "false" not in attribute_value.lower()

        for x in xml_element.childNodes:
            if x.nodeType != x.ELEMENT_NODE:
                continue
            if x.localName == "IsAbstract":
                self.isAbstract = "false" not in attribute_value.lower()


class MethodNode(Node):
    def __init__(self, xml_element=None):
        Node.__init__(self)
        self.executable = True
        self.userExecutable = True
        self.methodDeclaration = None
        if xml_element:
            MethodNode.parse_xml(self, xml_element)

    def parse_xml(self, xml_element):
        Node.parse_xml(self, xml_element)
        for attribute_type, attribute_value in xml_element.attributes.items():
            if attribute_type == "Executable":
                self.executable = "false" not in attribute_value.lower()
            if attribute_type == "UserExecutable":
                self.userExecutable = "false" not in attribute_value.lower()
            if attribute_type == "MethodDeclarationId":
                self.methodDeclaration = str(attribute_value)


class ObjectTypeNode(Node):
    def __init__(self, xml_element=None):
        Node.__init__(self)
        self.isAbstract = False
        if xml_element:
            ObjectTypeNode.parse_xml(self, xml_element)

    def parse_xml(self, xml_element):
        Node.parse_xml(self, xml_element)
        for (at, av) in xml_element.attributes.items():
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

    def __init__(self, xml_element=None):
        Node.__init__(self)
        self.isAbstract = False
        self.__xmlDefinition__ = None
        self.__baseTypeEncoding__ = []
        self.__encodable__ = None
        self.__definition__ = []
        self.__isEnum__ = False
        self.__isOptionSet__ = False
        if xml_element:
            DataTypeNode.parse_xml(self, xml_element)

    def parse_xml(self, xml_element):
        Node.parse_xml(self, xml_element)
        for at, av in xml_element.attributes.items():
            if at == "IsAbstract":
                self.isAbstract = "false" not in av.lower()

        for x in xml_element.childNodes:
            if x.nodeType == x.ELEMENT_NODE:
                if x.localName == "Definition":
                    self.__xmlDefinition__ = x

    def is_encodable(self):
        """ Will return True if buildEncoding() was able to determine which builtin
            type corresponds to all fields of this DataType.

            If no encoding has been build yet an exception will be thrown.
            Make sure to call buildEncoding() first.
        """
        if self.__encodable__ is None:
            raise Exception("Encoding needs to be built first using buildEncoding()")
        return self.__encodable__

    def get_encoding(self):
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

    def build_encoding(self, nodeset, indent=0, force=False):
        """ build_encoding() determines the structure and aliases used for variables
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

        if force:
            self.__encodable__ = None

        if self.__encodable__ is not None and self.__encodable__:
            if self.is_encodable():
                logger.debug(prefix + str(self.__baseTypeEncoding__) + " (already analyzed)")
            else:
                logger.debug(prefix + str(self.__baseTypeEncoding__) + "(already analyzed, not encodable!)")
            return self.__baseTypeEncoding__

        self.__encodable__ = True

        if indent == 0:
            logger.debug("Parsing DataType " + str(self.browseName) + " (" + str(self.id) + ")")

        if value_is_internal_type(self.browseName.name):
            self.__baseTypeEncoding__ = [self.browseName.name]
            self.__encodable__ = True
            logger.debug(prefix + str(self.browseName) + "*")
            logger.debug("Encodable as: " + str(self.__baseTypeEncoding__))
            logger.debug("")
            return self.__baseTypeEncoding__

        # Check if there is a supertype available
        parent_type = None
        target_node = None
        for ref in self.references:
            if ref.isForward:
                continue
                # hasSubtype
            if ref.referenceType.i == 45:
                target_node = nodeset.nodes[ref.target]
                if target_node is not None and isinstance(target_node, DataTypeNode):
                    parent_type = target_node
                    break

        if self.__xmlDefinition__ is None:
            if parent_type is not None:
                logger.debug(prefix + "Attempting definition using supertype " + str(target_node.browseName) +
                             " for DataType " + " " + str(self.browseName))
                subenc = target_node.build_encoding(nodeset=nodeset, indent=indent + 1)
                if not target_node.is_encodable():
                    self.__encodable__ = False
                else:
                    self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [self.browseName.name, subenc, None]
            if len(self.__baseTypeEncoding__) == 0:
                logger.debug(prefix + "No viable definition for " + str(self.browseName) + " " + str(self.id) +
                             " found.")
                self.__encodable__ = False

            if indent == 0:
                if not self.__encodable__:
                    logger.debug("Not encodable (partial): " + str(self.__baseTypeEncoding__))
                else:
                    logger.debug("Encodable as: " + str(self.__baseTypeEncoding__))
                logger.debug("")

            return self.__baseTypeEncoding__

        is_enum = True
        is_sub_type = True
        # An option set is at the same time also an enum, at least for the encoding below
        # TODO: remove magic number
        is_option_set = parent_type is not None and parent_type.id.ns == 0 and parent_type.id.i == 12755

        # We need to store the definition as ordered data, but can't use orderedDict
        # for backward compatibility with Python 2.6 and 3.4
        enum_dict = []
        type_dict = []

        # An XML Definition is provided and will be parsed... now
        for x in self.__xmlDefinition__.childNodes:
            if x.nodeType == x.ELEMENT_NODE:
                field_name = ""
                field_type = ""
                enum_val = ""
                value_rank = None
                # symbolicName = None
                for at, av in x.attributes.items():
                    if at == "DataType":
                        field_type = str(av)
                        if field_type in nodeset.aliases:
                            field_type = nodeset.aliases[field_type]
                        is_enum = False
                    elif at == "Name":
                        field_name = str(av)
                    # elif at == "SymbolicName":
                    #     symbolicName = str(av)
                    elif at == "Value":
                        enum_val = int(av)
                        is_sub_type = False
                    elif at == "ValueRank":
                        value_rank = int(av)
                    else:
                        logger.warning("Unknown Field Attribute " + str(at))
                # This can either be an enumeration OR a structure, not both.
                # Figure out which of the dictionaries gets the newly read value pair
                if is_enum == is_sub_type:
                    # This is an error
                    logger.warning("DataType contains both enumeration and subtype (or neither)")
                    self.__encodable__ = False
                    break
                elif is_enum:
                    # This is an enumeration
                    enum_dict.append((field_name, enum_val))
                    continue
                else:
                    if field_type == "":
                        # If no datatype given use base datatype
                        field_type = "i=24"

                    # This might be a subtype... follow the node defined as datatype to find out
                    # what encoding to use
                    fd_type_node_id = NodeId(field_type)
                    fd_type_node_id.ns = nodeset.namespace_mapping[self.modelUri][fd_type_node_id.ns]
                    if fd_type_node_id not in nodeset.nodes:
                        raise Exception("Node {} not found in nodeset".format(fd_type_node_id))
                    dtnode = nodeset.nodes[fd_type_node_id]
                    # The node in the datatype element was found. we inherit its encoding,
                    # but must still ensure that the dtnode is itself validly encodable
                    type_dict.append([field_name, dtnode])
                    field_type = str(dtnode.browseName.name)
                    logger.debug(prefix + field_name + " : " + field_type + " -> " + str(dtnode.id))
                    subenc = dtnode.build_encoding(nodeset=nodeset, indent=indent + 1)
                    self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [[field_name, subenc, value_rank]]
                    if not dtnode.is_encodable():
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

        if is_option_set:
            self.__isOptionSet__ = True
            subenc = parent_type.build_encoding(nodeset=nodeset)
            if not parent_type.is_encodable():
                self.__encodable__ = False
            else:
                self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + [self.browseName.name, subenc, None]
                self.__definition__ = enum_dict
            return self.__baseTypeEncoding__

        if is_enum:
            self.__baseTypeEncoding__ = self.__baseTypeEncoding__ + ['Int32']
            self.__definition__ = enum_dict
            self.__isEnum__ = True
            logger.debug(prefix + "Int32* -> enumeration with dictionary " + str(enum_dict) + " encodable " + str(
                self.__encodable__))
            return self.__baseTypeEncoding__

        if indent == 0:
            if not self.__encodable__:
                logger.debug("Not encodable (partial): " + str(self.__baseTypeEncoding__))
            else:
                logger.debug("Encodable as: " + str(self.__baseTypeEncoding__))
                self.__isEnum__ = False
                self.__definition__ = type_dict
            logger.debug("")
        return self.__baseTypeEncoding__


class ViewNode(Node):
    def __init__(self, xmlelement=None):
        Node.__init__(self)
        self.containsNoLoops = False
        self.eventNotifier = False
        if xmlelement:
            ViewNode.parse_xml(self, xmlelement)

    def parse_xml(self, xml_element):
        Node.parse_xml(self, xml_element)
        for (at, av) in xml_element.attributes.items():
            if at == "ContainsNoLoops":
                self.containsNoLoops = "false" not in av.lower()
            if at == "EventNotifier":
                self.eventNotifier = "false" not in av.lower()
