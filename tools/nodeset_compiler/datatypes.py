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
import re
from datetime import datetime
import xml.dom.minidom as dom
from base64 import *

__all__ = ['value_is_internal_type', 'Value', 'Boolean', 'Number', 'Integer',
           'UInteger', 'Byte', 'SByte',
           'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64', 'Float', 'Double',
           'String', 'XmlElement', 'ByteString', 'ExtensionObject', 'LocalizedText',
           'NodeId', 'ExpandedNodeId', 'DateTime', 'QualifiedName', 'StatusCode',
           'DiagnosticInfo', 'Guid']

logger = logging.getLogger(__name__)

if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s


    string_types = str
else:
    string_types = basestring


def get_next_element_node(xml_value):
    if xml_value is None:
        return None
    xml_value = xml_value.nextSibling
    while not xml_value is None and not xml_value.nodeType == xml_value.ELEMENT_NODE:
        xml_value = xml_value.nextSibling
    return xml_value


def value_is_internal_type(value_type_string):
    return value_type_string.lower() in ['boolean', 'number', 'int32', 'uint32', 'int16', 'uint16',
                                         'int64', 'uint64', 'byte', 'sbyte', 'float', 'double',
                                         'string', 'bytestring', 'localizedtext', 'statuscode',
                                         'diagnosticinfo', 'nodeid', 'guid', 'datetime',
                                         'qualifiedname', 'expandednodeid', 'xmlelement', 'integer', 'uinteger']


class Value(object):
    def __init__(self):
        self.value = None
        self.alias = None
        self.dataType = None
        self.encodingRule = []
        self.isInternal = False
        self.valueRank = None

    def get_value_field_by_alias(self, field_name):
        if not isinstance(self.value, list):
            return None
        if not isinstance(self.value[0], Value):
            return None
        for val in self.value:
            if val.alias() == field_name:
                return val.value
        return None

    def get_type_by_string(self, string_name, encoding_rule):
        string_name = str(string_name.lower())
        if string_name == 'boolean':
            t = Boolean()
        elif string_name == 'number':
            t = Number()
        elif string_name == 'integer':
            t = Integer()
        elif string_name == 'uinteger':
            t = UInteger()
        elif string_name == 'int32':
            t = Int32()
        elif string_name == 'uint32':
            t = UInt32()
        elif string_name == 'int16':
            t = Int16()
        elif string_name == 'uint16':
            t = UInt16()
        elif string_name == 'int64':
            t = Int64()
        elif string_name == 'uint64':
            t = UInt64()
        elif string_name == 'byte':
            t = Byte()
        elif string_name == 'sbyte':
            t = SByte()
        elif string_name == 'float':
            t = Float()
        elif string_name == 'double':
            t = Double()
        elif string_name == 'string':
            t = String()
        elif string_name == 'bytestring':
            t = ByteString()
        elif string_name == 'localizedtext':
            t = LocalizedText()
        elif string_name == 'statuscode':
            t = StatusCode()
        elif string_name == 'diagnosticinfo':
            t = DiagnosticInfo()
        elif string_name == 'nodeid':
            t = NodeId()
        elif string_name == 'guid':
            t = Guid()
        elif string_name == 'datetime':
            t = DateTime()
        elif string_name == 'qualifiedname':
            t = QualifiedName()
        elif string_name == 'expandednodeid':
            t = ExpandedNodeId()
        elif string_name == 'xmlelement':
            t = XmlElement()
        else:
            logger.debug("No class representing stringName " + string_name + " was found. Cannot create builtinType.")
            return None
        t.encodingRule = encoding_rule
        return t

    def check_xml(self, xml_value):
        if xml_value is None or xml_value.nodeType != xml_value.ELEMENT_NODE:
            logger.error("Expected XML Element, but got junk...")
            return

    def parse_xml_encoding(self, xml_value, parent_data_type_node, parent, namespace_mapping):
        self.check_xml(xml_value)
        if "value" not in xml_value.localName.lower():
            logger.error("Expected <Value> , but found " + xml_value.localName + \
                         " instead. Value will not be parsed.")
            return

        if len(xml_value.childNodes) == 0:
            logger.error("Expected childnodes for value, but none were found...")
            return

        for n in xml_value.childNodes:
            if n.nodeType == n.ELEMENT_NODE:
                xml_value = n
                break

        if "ListOf" in xml_value.localName:
            self.value = []
            for el in xml_value.childNodes:
                if not el.nodeType == el.ELEMENT_NODE:
                    continue
                val = self.__parse_xml_single_value(el, parent_data_type_node, parent,
                                                    namespace_mapping=namespace_mapping)
                if val is None:
                    self.value = []
                    return
                self.value.append(val)
        else:
            self.value = [
                self.__parse_xml_single_value(xml_value, parent_data_type_node, parent,
                                              namespace_mapping=namespace_mapping)]

    def __parse_xml_single_value(self, xml_value, parent_data_type_node, parent, namespace_mapping, alias=None,
                                 encoding_part=None, value_rank=None):
        # Parse an encoding list such as enc = [[Int32], ['Duration', ['DateTime']]],
        # returning a possibly aliased variable or list of variables.
        # Keep track of aliases, as ['Duration', ['Hawaii', ['UtcTime', ['DateTime']]]]
        # will be of type DateTime, but tagged as <Duration>2013-04-10 12:00 UTC</Duration>,
        # and not as <Duration><Hawaii><UtcTime><String>2013-04-10 12:00 UTC</String>...

        # Encoding may be partially handed down (iterative call). Only resort to
        # type definition if we are not given a specific encoding to match
        if encoding_part is None:
            enc = parent_data_type_node.get_encoding()
        else:
            enc = encoding_part

        # Check the structure of the encoding list to determine if a type is to be
        # returned or we need to descend further checking aliases or multipart types
        # such as extension Objects.
        if len(enc) == 1:
            # 0: ['BuiltinType']          either builtin type
            # 1: [ [ 'Alias', [...], n] ] or single alias for possible multipart
            if isinstance(enc[0], string_types):
                # 0: 'BuiltinType'
                if alias is not None:
                    if xml_value is not None and not xml_value.localName == alias and not xml_value.localName == enc[0]:
                        logger.error(str(
                            parent.id) + ": Expected XML element with tag " + alias + " but found " + xml_value.localName + " instead")
                        return None
                    else:
                        t = self.get_type_by_string(enc[0], enc)
                        t.alias = alias
                        t.valueRank = value_rank

                        if value_rank == 1:
                            values = []
                            for el in xml_value.childNodes:
                                if not el.nodeType == el.ELEMENT_NODE:
                                    continue
                                val = self.get_type_by_string(enc[0], enc)
                                val.parse_xml(el, namespace_mapping=namespace_mapping)
                                values.append(val)
                            return values
                        else:
                            if xml_value is not None:
                                t.parse_xml(xml_value, namespace_mapping=namespace_mapping)
                            return t
                else:
                    if not value_is_internal_type(xml_value.localName):
                        logger.error(str(parent.id) + ": Expected XML describing builtin type " + enc[
                            0] + " but found " + xml_value.localName + " instead")
                    else:
                        t = self.get_type_by_string(enc[0], enc)
                        t.parse_xml(xml_value, namespace_mapping=namespace_mapping)
                        t.isInternal = True
                        return t
            else:
                # 1: ['Alias', [...], n]
                # Let the next elif handle this
                return self.__parse_xml_single_value(xml_value, parent_data_type_node, parent,
                                                     namespace_mapping=namespace_mapping,
                                                     alias=alias, encoding_part=enc[0],
                                                     value_rank=enc[2] if len(enc) > 2 else None)
        elif len(enc) == 3 and isinstance(enc[0], string_types):
            # [ 'Alias', [...], 0 ]          aliased multipart
            if alias is None:
                alias = enc[0]
            # if we have an alias and the next field is multipart, keep the alias
            elif alias is not None and len(enc[1]) > 1:
                alias = enc[0]
            # otherwise drop the alias
            return self.__parse_xml_single_value(xml_value, parent_data_type_node, parent,
                                                 namespace_mapping=namespace_mapping,
                                                 alias=alias, encoding_part=enc[1],
                                                 value_rank=enc[2] if len(enc) > 2 else None)
        else:
            # [ [...], [...], [...]] multifield of unknowns (analyse separately)
            # create an extension object to hold multipart type

            # FIXME: This implementation expects an extensionobject to be manditory for
            #        multipart variables. Variants/Structures are not included in the
            #        OPCUA Namespace 0 nodeset.
            #        Consider moving this ExtensionObject specific parsing into the
            #        builtin type and only determining the multipart type at this stage.
            if not xml_value.localName == "ExtensionObject":
                logger.error(str(
                    parent.id) + ": Expected XML tag <ExtensionObject> for multipart type, but found " + xml_value.localName + " instead.")
                return None

            extension_object = ExtensionObject()
            extension_object.encodingRule = enc
            element_type = xml_value.getElementsByTagName("TypeId")
            if len(element_type) == 0:
                logger.error(str(parent.id) + ": Did not find <TypeId> for ExtensionObject")
                return None
            element_type = element_type[0].getElementsByTagName("Identifier")
            if len(element_type) == 0:
                logger.error(str(parent.id) + ": Did not find <Identifier> for ExtensionObject")
                return None

            element_type = NodeId(element_type[0].firstChild.data.strip(' \t\n\r'))
            extension_object.typeId = element_type

            body = xml_value.getElementsByTagName("Body")
            if len(body) == 0:
                logger.error(str(parent.id) + ": Did not find <Body> for ExtensionObject")
                return None
            body = body[0]

            # Body must contain an Object of type 'DataType' as defined in Variable
            body_part = body.firstChild
            if not body_part.nodeType == body_part.ELEMENT_NODE:
                body_part = get_next_element_node(body_part)
            if body_part is None:
                logger.error(str(parent.id) + ": Expected ExtensionObject to hold a variable of type " + str(
                    parent_data_type_node.browseName) + " but found nothing.")
                return None

            if not body_part.localName == "OptionSet":
                if not body_part.localName == parent_data_type_node.browseName.name:
                    logger.error(str(parent.id) + ": Expected ExtensionObject to hold a variable of type " + str(
                        parent_data_type_node.browseName) + " but found " +
                                 str(body_part.localName) + " instead.")
                    return None
            extension_object.alias = body_part.localName

            body_part = body_part.firstChild
            if not body_part.nodeType == body_part.ELEMENT_NODE:
                body_part = get_next_element_node(body_part)
            if body_part is None:
                logger.error(str(parent.id) + ": Description of dataType " + str(
                    parent_data_type_node.browseName) + " in ExtensionObject is empty/invalid.")
                return None

            extension_object.value = []
            for e in enc:
                extension_object.value.append(
                    extension_object.__parseXMLSingleValue(body_part, parent_data_type_node, parent,
                                                           namespaceMapping=namespace_mapping, alias=None,
                                                           encodingPart=e))
                body_part = get_next_element_node(body_part)
            return extension_object

    def __str__(self):
        return self.__class__.__name__ + "(" + str(self.value) + ")"

    def is_none(self):
        return self.value is None

    def __repr__(self):
        return self.__str__()


#################
# Builtin Types #
#################


def get_xml_text_trimmed(xml_node):
    if xml_node is None or xml_node.data is None:
        return None
    content = xml_node.data
    # Check for empty string (including newlines)
    if not re.sub(r"[\s\n\r]", "", content).strip():
        return None
    return unicode(content.strip())


class Boolean(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <Boolean>value</Boolean> or
        #        <Aliasname>value</Aliasname>
        self.check_xml(xml_value)
        val = get_xml_text_trimmed(xml_value.firstChild)
        if val is None:
            self.value = "false"  # Catch XML <Boolean /> by setting the value to a default
        else:
            if "false" in unicode(xml_value.firstChild.data).lower():
                self.value = "false"
            else:
                self.value = "true"


class Number(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <Int16>value</Int16> or any other valid number type, or
        #        <Aliasname>value</Aliasname>
        self.check_xml(xml_value)
        val = get_xml_text_trimmed(xml_value.firstChild)
        self.value = val if val is not None else 0


class Integer(Number):
    def __init__(self, xml_element=None):
        Number.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class UInteger(Number):
    def __init__(self, xml_element=None):
        Number.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class Byte(UInteger):
    def __init__(self, xml_element=None):
        UInteger.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class SByte(Integer):
    def __init__(self, xml_element=None):
        Integer.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class Int16(Integer):
    def __init__(self, xml_element=None):
        Integer.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class UInt16(UInteger):
    def __init__(self, xml_element=None):
        UInteger.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class Int32(Integer):
    def __init__(self, xml_element=None):
        Integer.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class UInt32(UInteger):
    def __init__(self, xml_element=None):
        UInteger.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class Int64(Integer):
    def __init__(self, xml_element=None):
        Integer.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class UInt64(UInteger):
    def __init__(self, xml_element=None):
        UInteger.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class Float(Number):
    def __init__(self, xml_element=None):
        Number.__init__(self)
        if xml_element:
            Float.parse_xml(self, xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <Float>value</Float> or
        #        <Aliasname>value</Aliasname>
        self.check_xml(xml_value)
        val = get_xml_text_trimmed(xml_value.firstChild)
        self.value = val if val is not None else 0.0


class Double(Float):
    def __init__(self, xml_element=None):
        Float.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)


class String(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def pack(self):
        binary = structpack("I", len(unicode(self.value)))
        binary = binary + str(self.value)
        return binary

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <String>value</String> or
        #        <Aliasname>value</Aliasname>
        if not isinstance(xml_value, dom.Element):
            self.value = xml_value
            return
        self.check_xml(xml_value)
        val = get_xml_text_trimmed(xml_value.firstChild)
        self.value = val if val is not None else ""


class XmlElement(String):
    def __init__(self, xml_element=None):
        String.__init__(self, xml_element)


class ByteString(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <ByteString>value</ByteString>
        if not isinstance(xml_value, dom.Element):
            self.value = xml_value
            return
        self.check_xml(xml_value)
        if xml_value.firstChild is None:
            self.value = []  # Catch XML <ByteString /> by setting the value to a default
        else:
            self.value = b64decode(xml_value.firstChild.data)


class ExtensionObject(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_element, namespace_mapping=None):
        pass

    def __str__(self):
        return "'ExtensionObject'"


class LocalizedText(Value):
    def __init__(self, xml_value=None):
        Value.__init__(self)
        self.locale = None
        self.text = None
        if xml_value:
            self.parse_xml(xml_value)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <LocalizedText> or <AliasName>
        #          <Locale>xx_XX</Locale>
        #          <Text>TextText</Text>
        #        <LocalizedText> or </AliasName>
        if not isinstance(xml_value, dom.Element):
            self.text = xml_value
            return
        self.check_xml(xml_value)
        tmp = xml_value.getElementsByTagName("Locale")
        if len(tmp) > 0 and tmp[0].firstChild is not None:
            self.locale = tmp[0].firstChild.data.strip(' \t\n\r')
        tmp = xml_value.getElementsByTagName("Text")
        if len(tmp) > 0 and tmp[0].firstChild is not None:
            self.text = tmp[0].firstChild.data.strip(' \t\n\r')

    def __str__(self):
        if self.locale is None and self.text is None:
            return "None"
        if self.locale is not None and len(self.locale) > 0:
            return "(" + self.locale + ":" + self.text + ")"
        else:
            return self.text

    def is_none(self):
        return self.text is None


class NodeId(Value):
    def __init__(self, id_string=None):
        Value.__init__(self)
        self.i = None
        self.b = None
        self.g = None
        self.s = None
        self.ns = 0
        self.text = None
        self.set_from_id_string(id_string)

    def set_from_id_string(self, id_string):

        if not id_string:
            self.i = 0
            return

        # The ID will encoding itself appropriatly as string. If multiple ID's
        # (numeric, string, guid) are defined, the order of preference for the ID
        # string is always numeric, guid, bytestring, string. Binary encoding only
        # applies to numeric values (UInt16).
        id_parts = id_string.strip().split(";")
        for p in id_parts:
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
                    i = "0x" + i
                    tmp.append(int(i, 16))
                self.g = tmp
            elif p[:2] == "s=":
                self.s = p[2:]
            else:
                raise Exception("no valid nodeid: " + id_string)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <NodeId> or <Alias>
        #           <Identifier> # It is unclear whether or not this is manditory.
        #                          Identifier tags are used in Namespace 0.
        #                ns=x;i=y or similar string representation of id()
        #           </Identifier>
        #        </NodeId> or </Alias>
        if not isinstance(xml_value, dom.Element):
            self.text = xml_value  # Alias
            return
        self.check_xml(xml_value)

        # Catch XML <NodeId />
        if xml_value.firstChild is None:
            logger.error("No value is given, which is illegal for Node Types...")
            self.value = None
        else:
            # Check if there is an <Identifier> tag
            if len(xml_value.getElementsByTagName("Identifier")) != 0:
                xml_value = xml_value.getElementsByTagName("Identifier")[0]
            self.set_from_id_string(unicode(xml_value.firstChild.data))
            if namespace_mapping is not None:
                self.ns = namespace_mapping[self.ns]

    def __str__(self):
        s = "ns=" + str(self.ns) + ";"
        # Order of preference is numeric, guid, bytestring, string
        if self.i is not None:
            return s + "i=" + str(self.i)
        elif self.g is not None:
            s = s + "g="
            tmp = []
            for i in self.g:
                tmp.append(hex(i).replace("0x", ""))
            for i in tmp:
                s = s + "-" + i
            return s.replace("g=-", "g=")
        elif self.b is not None:
            return s + "b=" + str(self.b)
        elif self.s is not None:
            return s + "s=" + str(self.s)

    def is_none(self):
        return self.i is None and self.b is None and self.s is None and self.g is None

    def __eq__(self, node_id2):
        return str(self) == str(node_id2)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        return str(self)

    def __hash__(self):
        return hash(str(self))


class ExpandedNodeId(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        self.check_xml(xml_value)
        logger.debug("Not implemented", LOG_LEVEL_ERR)


class DateTime(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <DateTime> or <AliasName>
        #        2013-08-13T21:00:05.0000L
        #        </DateTime> or </AliasName>
        self.check_xml(xml_value)
        time_str = get_xml_text_trimmed(xml_value.firstChild)

        if time_str is None:
            # Catch XML <DateTime /> by setting the value to a default
            self.value = datetime(2001, 1, 1)
        else:
            # .NET tends to create this garbage %Y-%m-%dT%H:%M:%S.0000z
            # strip everything after the "." away for a posix time_struct
            if "." in time_str:
                time_str = time_str[:time_str.index(".")]
            # If the last character is not numeric, remove it
            while len(time_str) > 0 and not time_str[-1] in "0123456789":
                time_str = time_str[:-1]
            try:
                self.value = datetime.strptime(time_str, "%Y-%m-%dT%H:%M:%S")
            except Exception:
                try:
                    self.value = datetime.strptime(time_str, "%Y-%m-%d")
                except Exception:
                    logger.error(
                        "Timestring format is illegible. Expected 2001-01-30T21:22:23 or 2001-01-30, but got " + \
                        time_str + " instead. Time will be defaultet to now()")
                    self.value = datetime(2001, 1, 1)


class QualifiedName(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        self.ns = 0
        self.name = None
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        # Expect <QualifiedName> or <AliasName>
        #           <NamespaceIndex>Int16<NamespaceIndex>
        #           <Name>SomeString<Name>
        #        </QualifiedName> or </AliasName>
        if not isinstance(xml_value, dom.Element):
            colon_index = xml_value.find(":")
            if colon_index == -1:
                self.name = xml_value
            else:
                self.name = xml_value[colon_index + 1:]
                self.ns = int(xml_value[:colon_index])
                if namespace_mapping is not None:
                    self.ns = namespace_mapping[self.ns]
            return

        self.check_xml(xml_value)
        # Is a namespace index passed?
        if len(xml_value.getElementsByTagName("NamespaceIndex")) != 0:
            self.ns = int(xml_value.getElementsByTagName("NamespaceIndex")[0].firstChild.data)
            if namespace_mapping is not None:
                self.ns = namespace_mapping[self.ns]
        if len(xml_value.getElementsByTagName("Name")) != 0:
            self.name = xml_value.getElementsByTagName("Name")[0].firstChild.data

    def __str__(self):
        return "ns=" + str(self.ns) + ";" + str(self.name)

    def is_none(self):
        return self.name is None


class StatusCode(UInt32):
    def __init__(self, xml_element=None):
        UInt32.__init__(self, xml_element)


class DiagnosticInfo(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        self.check_xml(xml_value)
        logger.warn("Not implemented")


class Guid(Value):
    def __init__(self, xml_element=None):
        Value.__init__(self)
        if xml_element:
            self.parse_xml(xml_element)

    def parse_xml(self, xml_value, namespace_mapping=None):
        self.check_xml(xml_value)

        val = get_xml_text_trimmed(xml_value.firstChild)

        if val is None:
            self.value = [0, 0, 0, 0]  # Catch XML <Guid /> by setting the value to a default
        else:
            self.value = val
            self.value = self.value.replace("{", "")
            self.value = self.value.replace("}", "")
            self.value = self.value.split("-")
            tmp = []
            for g in self.value:
                try:
                    tmp.append(int("0x" + g, 16))
                except Exception:
                    logger.error("Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + \
                                 unicode(xml_value.firstChild.data))
                    tmp = [0, 0, 0, 0, 0]
            if len(tmp) != 5:
                logger.error("Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + \
                             unicode(xml_value.firstChild.data))
                tmp = [0, 0, 0, 0]
            self.value = tmp
