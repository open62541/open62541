#!/usr/bin/env python3
# -*- coding: utf-8 -*-

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH


import sys
import logging
import re
from datetime import datetime
import xml.dom.minidom as dom
from base64 import b64decode

from type_parser import BuiltinType, EnumerationType, StructMember, StructType

""" __all__ = ['valueIsInternalType', 'Value', 'Boolean', 'Number', 'Integer',
           'UInteger', 'Byte', 'SByte',
           'Int16', 'UInt16', 'Int32', 'UInt32', 'Int64', 'UInt64', 'Float', 'Double',
           'String', 'XmlElement', 'ByteString', 'Structure', 'ExtensionObject', 'LocalizedText',
           'NodeId', 'ExpandedNodeId', 'DateTime', 'QualifiedName', 'StatusCode',
           'DiagnosticInfo', 'Guid'] """

logger = logging.getLogger(__name__)

namespaceMapping = {}

if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s

    string_types = str
else:
    string_types = basestring 

def getNextElementNode(xmlvalue):
    if xmlvalue is None:
        return None
    xmlvalue = xmlvalue.nextSibling
    while not xmlvalue is None and not xmlvalue.nodeType == xmlvalue.ELEMENT_NODE:
        xmlvalue = xmlvalue.nextSibling
    return xmlvalue

def valueIsInternalType(valueTypeString):
    return valueTypeString.lower() in ['boolean', 'number', 'int32', 'uint32', 'int16', 'uint16',
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

    def getValueFieldByAlias(self, fieldname):
        if not isinstance(self.value, list):
            return None
        if not isinstance(self.value[0], Value):
            return None
        for val in self.value:
            if val.alias() == fieldname:
                return val.value
        return None

    def getTypeByString(self, stringName, encodingRule):
        stringName = str(stringName.lower())
        if stringName == 'boolean':
            t = Boolean()
        elif stringName == 'number':
            t = Number()
        elif stringName == 'integer':
            t = Integer()
        elif stringName == 'uinteger':
            t = UInteger()
        elif stringName == 'int32':
            t = Int32()
        elif stringName == 'uint32':
            t = UInt32()
        elif stringName == 'int16':
            t = Int16()
        elif stringName == 'uint16':
            t = UInt16()
        elif stringName == 'int64':
            t = Int64()
        elif stringName == 'uint64':
            t = UInt64()
        elif stringName == 'byte':
            t = Byte()
        elif stringName == 'sbyte':
            t = SByte()
        elif stringName == 'float':
            t = Float()
        elif stringName == 'double':
            t = Double()
        elif stringName == 'string':
            t = String()
        elif stringName == 'bytestring':
            t = ByteString()
        elif stringName == 'localizedtext':
            t = LocalizedText()
        elif stringName == 'statuscode':
            t = StatusCode()
        elif stringName == 'diagnosticinfo':
            t = DiagnosticInfo()
        elif stringName == 'nodeid':
            t = NodeId()
        elif stringName == 'guid':
            t = Guid()
        elif stringName == 'datetime':
            t = DateTime()
        elif stringName == 'qualifiedname':
            t = QualifiedName()
        elif stringName == 'expandednodeid':
            t = ExpandedNodeId()
        elif stringName == 'xmlelement':
            t = XmlElement()
        else:
            logger.debug("No class representing stringName " + stringName + " was found. Cannot create builtinType.")
            return None
        t.encodingRule = encodingRule
        return t

    def checkXML(self, xmlvalue):
        if xmlvalue is None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
            logger.error("Expected XML Element, but got junk...")
            return

    def parseXMLEncoding(self, xmlvalue, parentDataTypeNode, parent, parser):
        global namespaceMapping
        self.checkXML(xmlvalue)
        if not "value" in xmlvalue.localName.lower():
            logger.error("Expected <Value> , but found " + xmlvalue.localName + \
                         " instead. Value will not be parsed.")
            return

        if len(xmlvalue.childNodes) == 0:
            logger.error("Expected childnodes for value, but none were found...")
            return

        for n in xmlvalue.childNodes:
            if n.nodeType == n.ELEMENT_NODE:
                xmlvalue = n
                break

        namespaceMapping = parent.namespaceMapping
        if "ListOf" in xmlvalue.localName:
            self.value = []
            for el in xmlvalue.childNodes:
                if not el.nodeType == el.ELEMENT_NODE:
                    continue
                val = self.__parseXMLSingleValue(el, parentDataTypeNode, parent, parser)
                if val is None:
                    self.value = []
                    namespaceMapping = {}
                    return
                self.value.append(val)
        else:
            self.value = [self.__parseXMLSingleValue(xmlvalue, parentDataTypeNode, parent, parser)]
            namespaceMapping = {}

    def __parseXMLSingleValue(self, xmlvalue, parentDataTypeNode, parent, parser, alias=None, encodingPart=None, valueRank=None):
        enc = None
        if encodingPart is None:
            if not parentDataTypeNode.symbolicName is None:
                for _, e in parser.types.items():
                    if not enc is None:
                        break
                    for key, value in e.items():
                        # Inside the parser are the symbolic names of the data types. If the display name and symbolic name are different, both must be checked.
                        # An example is the 3DFrame datatype where the symbolic name is ThreeDFrame.
                        if key == parentDataTypeNode.displayName.text or key == parentDataTypeNode.symbolicName.value:
                            enc = value
                            break
            else:
                for _, e in parser.types.items():
                    if not enc is None:
                        break
                    for key, value in e.items():
                        if key == parentDataTypeNode.displayName.text:
                            enc = value
                            break
        else:
            enc = encodingPart
            ebodypart = xmlvalue

        if xmlvalue.localName == "ExtensionObject":
            extobj = ExtensionObject()

            etype = xmlvalue.getElementsByTagName("TypeId")
            if len(etype) == 0:
                logger.error(str(parent.id) + ": Did not find <TypeId> for ExtensionObject")
                return extobj
            etype = etype[0].getElementsByTagName("Identifier")
            if len(etype) == 0:
                logger.error(str(parent.id) + ": Did not find <Identifier> for ExtensionObject")
                return extobj

            etype = NodeId(etype[0].firstChild.data.strip(' \t\n\r'))
            extobj.typeId = etype

            ebody = xmlvalue.getElementsByTagName("Body")
            if len(ebody) == 0:
                logger.error(str(parent.id) + ": Did not find <Body> for ExtensionObject")
                return extobj
            ebody = ebody[0]

            try:
                # Body must contain an Object of type 'DataType' as defined in Variable
                ebodypart = ebody.firstChild
                if not ebodypart.nodeType == ebodypart.ELEMENT_NODE:
                    ebodypart = getNextElementNode(ebodypart)
                if ebodypart is None:
                    logger.error(str(parent.id) + ": Expected ExtensionObject to hold a variable of type " + str(parentDataTypeNode.browseName) + " but found nothing.")
                    return extobj

                parentName = parentDataTypeNode.browseName.name
                if parentDataTypeNode.symbolicName is not None and parentDataTypeNode.symbolicName.value is not None:
                    parentName = parentDataTypeNode.symbolicName.value
                if not ebodypart.localName == "OptionSet" and not ebodypart.localName == parentName:
                    logger.error(   str(parent.id) + ": Expected ExtensionObject to hold a variable of type " + str(parentDataTypeNode.browseName) + " but found " +
                                    str(ebodypart.localName) + " instead.")
                    return extobj
                extobj.alias = ebodypart.localName

                ebodypart = ebodypart.firstChild
                if not ebodypart.nodeType == ebodypart.ELEMENT_NODE:
                    ebodypart = getNextElementNode(ebodypart)
                if ebodypart is None:
                    logger.error(str(parent.id) + ": Description of dataType " + str(parentDataTypeNode.browseName) + " in ExtensionObject is empty/invalid.")
                    return extobj

                extobj.value = []
                members = enc.members

                # The EncodingMask must be skipped.
                if ebodypart.localName == "EncodingMask":
                    ebodypart = getNextElementNode(ebodypart)

                # The SwitchField must be checked.
                if ebodypart.localName == "SwitchField":
                    # The switch field is the index of the available union fields starting with 1
                    data = int(ebodypart.firstChild.data)
                    if data == 0:
                        # If the switch field is 0 then no field is present. A Union with no fields present has the same meaning as a NULL value.
                        members = []
                    else:
                        members = []
                        members.append(enc.members[data-1])
                        ebodypart = getNextElementNode(ebodypart)


                for e in members:
                    # ebodypart can be None if the field is not set, although the field is not optional.
                    if ebodypart is None:
                        if not e.is_optional:
                            t = self.getTypeByString(e.member_type.name, None)
                            extobj.value.append(t)
                            extobj.encodingRule.append(e)
                        continue
                    if isinstance(e, StructMember):
                        if not e.name.lower() == ebodypart.localName.lower():
                            continue
                        extobj.encodingRule.append(e)
                        if isinstance(e.member_type, BuiltinType):
                            if e.is_array:
                                values = []
                                for el in ebodypart.childNodes:
                                    if not el.nodeType == el.ELEMENT_NODE:
                                        continue
                                    t = self.getTypeByString(e.member_type.name, None)
                                    t.parseXML(el)
                                    values.append(t)
                                extobj.value.append(values)
                            else:
                                t = self.getTypeByString(e.member_type.name, None)
                                t.alias = ebodypart.localName
                                t.parseXML(ebodypart)
                                extobj.value.append(t)
                        elif isinstance(e.member_type, StructType):
                            # information is_array!
                            structure = Structure()
                            structure.alias = ebodypart.localName
                            structure.value = []
                            if e.is_array:
                                values = []
                                for el in ebodypart.childNodes:
                                    if not el.nodeType == el.ELEMENT_NODE:
                                        continue
                                    structure.__parseXMLSingleValue(el, parentDataTypeNode, parent, parser, alias=None, encodingPart=e.member_type)
                                    values.append(structure.value)
                                    structure.value = []
                                structure.value = values
                            else:
                                structure.__parseXMLSingleValue(ebodypart, parentDataTypeNode, parent, parser, alias=None, encodingPart=e.member_type)
                            extobj.value.append(structure)
                        elif isinstance(e.member_type, EnumerationType):
                            t = self.getTypeByString("Int32", None)
                            t.parseXML(ebodypart)
                            extobj.value.append(t)
                        else:
                            logger.error(str(parent.id) + ": Description of dataType " + str(parentDataTypeNode.browseName) + " in ExtensionObject is not a BuildinType, StructType or EnumerationType.")
                            return extobj
                    else:
                            logger.error(str(parent.id) + ": Description of dataType " + str(parentDataTypeNode.browseName) + " in ExtensionObject is not a StructMember.")
                            return extobj

                    ebodypart = getNextElementNode(ebodypart)

            except Exception as ex:
                logger.error(str(parent.id) + ": Could not parse <Body> for ExtensionObject. {}".format(ex))

        elif valueIsInternalType(xmlvalue.localName):
            t = self.getTypeByString(xmlvalue.localName, None)
            t.parseXML(xmlvalue)
            t.isInternal = True
            return t
        elif isinstance(enc, StructType):
            members = enc.members
            # The StructType can be a union and must be handled.
            if enc.is_union:
                body = xmlvalue.getElementsByTagName("SwitchField")
                body = body[0]
                # The switch field is the index of the available union fields starting with 1
                if body.localName == "SwitchField":
                    data = int(body.firstChild.data)
                    if data == 0:
                        # If the switch field is 0 then no field is present. A Union with no fields present has the same meaning as a NULL value.
                        return None
                    else:
                        members = []
                        members.append(enc.members[data-1])
                        ebodypart = getNextElementNode(body)
                else:
                    logger.error(str(parent.id) + ": Could not parse <SwitchFiled> for Union.")
                    return self
                

            childValue = ebodypart.firstChild
            if not childValue.nodeType == ebodypart.ELEMENT_NODE:
                childValue = getNextElementNode(childValue)
            for e in members:
                    if isinstance(e, StructMember):
                        self.encodingRule.append(e)
                        if isinstance(e.member_type, BuiltinType):
                            if e.is_array:
                                values = []
                                for el in childValue.childNodes:
                                    if not el.nodeType == el.ELEMENT_NODE:
                                        continue
                                    t = self.getTypeByString(e.member_type.name, None)
                                    t.parseXML(el)
                                    values.append(t)
                                self.value.append(values)
                            else:
                                t = self.getTypeByString(e.member_type.name, None)
                                t.alias = e.name
                                if childValue is not None:
                                    t.parseXML(childValue)
                                    self.value.append(t)
                                else:
                                    if not e.is_optional:
                                        self.value.append(t)
                        elif isinstance(e.member_type, StructType):
                            structure = Structure()
                            structure.alias = e.name
                            structure.value = []
                            if not len(childValue.childNodes) == 0:
                                structure.__parseXMLSingleValue(childValue, parentDataTypeNode, parent, parser, alias=None, encodingPart=e.member_type)
                            self.value.append(structure)
                            return structure
                        elif isinstance(e.member_type, EnumerationType):
                            t = self.getTypeByString("Int32", None)
                            t.parseXML(childValue)
                            t.alias = e.name
                            self.value.append(t)
                        else:
                            logger.error(str(parent.id) + ": Description of dataType " + str(parentDataTypeNode.browseName) + " in ExtensionObject is not a BuildinType, EnumerationType or StructMember.")
                            return self

                        childValue = getNextElementNode(childValue)
            return self

        return extobj

    def __str__(self):
        return self.__class__.__name__ + "(" + str(self.value) + ")"

    def isNone(self):
        return self.value is None

    def __repr__(self):
        return self.__str__()

#################
# Builtin Types #
#################


def getXmlTextTrimmed(xmlNode):
    if xmlNode is None or xmlNode.data is None:
        return None
    content = xmlNode.data
    # Check for empty string (including newlines)
    if not re.sub(r"[\s\n\r]", "", content).strip():
        return None
    return unicode(content.strip())


class Boolean(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        # Expect <Boolean>value</Boolean> or
        #        <Aliasname>value</Aliasname>
        self.checkXML(xmlvalue)
        val = getXmlTextTrimmed(xmlvalue.firstChild)
        if val is None:
            self.value = "false"  # Catch XML <Boolean /> by setting the value to a default
        else:
            if "false" in unicode(xmlvalue.firstChild.data).lower():
                self.value = "false"
            else:
                self.value = "true"

class Number(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        # Expect <Int16>value</Int16> or any other valid number type, or
        #        <Aliasname>value</Aliasname>
        self.checkXML(xmlvalue)
        val = getXmlTextTrimmed(xmlvalue.firstChild)
        self.value = val if val is not None else 0

class Integer(Number):
    def __init__(self, xmlelement=None):
        Number.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class UInteger(Number):
    def __init__(self, xmlelement=None):
        Number.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class Byte(UInteger):
    def __init__(self, xmlelement=None):
        UInteger.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class SByte(Integer):
    def __init__(self, xmlelement=None):
        Integer.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class Int16(Integer):
    def __init__(self, xmlelement=None):
        Integer.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class UInt16(UInteger):
    def __init__(self, xmlelement=None):
        UInteger.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class Int32(Integer):
    def __init__(self, xmlelement=None):
        Integer.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        Integer.parseXML(self, xmlvalue)
        # Values of enumerations can be encoded as strings: <symbol>_<value> (see OPC specification part 6)
        # UaModeler does this for enums that are fields of structs
        # Extract <value> from string if possible
        if isinstance(self.value, string_types) and not self.__strIsInt(self.value):
            split = self.value.split('_')
            if len(split) == 2 and self.__strIsInt(split[1]):
                self.value = split[1]

    @staticmethod
    def __strIsInt(strValue):
        try:
            int(strValue)
            return True
        except:
            return False

class UInt32(UInteger):
    def __init__(self, xmlelement=None):
        UInteger.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class Int64(Integer):
    def __init__(self, xmlelement=None):
        Integer.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class UInt64(UInteger):
    def __init__(self, xmlelement=None):
        UInteger.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class Float(Number):
    def __init__(self, xmlelement=None):
        Number.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        # Expect <Float>value</Float> or
        #        <Aliasname>value</Aliasname>
        self.checkXML(xmlvalue)
        val = getXmlTextTrimmed(xmlvalue.firstChild)
        self.value = val if val is not None else 0.0

class Double(Float):
    def __init__(self, xmlelement=None):
        Float.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

class String(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def pack(self):
        bin = structpack("I", len(unicode(self.value)))
        bin = bin + str(self.value)
        return bin

    def parseXML(self, xmlvalue):
        # Expect <String>value</String> or
        #        <Aliasname>value</Aliasname>
        if not isinstance(xmlvalue, dom.Element):
            self.value = xmlvalue
            return
        self.checkXML(xmlvalue)
        val = getXmlTextTrimmed(xmlvalue.firstChild)
        self.value = val if val is not None else ""


class XmlElement(String):
    def __init__(self, xmlelement=None):
        String.__init__(self, xmlelement)

class ByteString(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)

    def parseXML(self, xmlvalue):
        # Expect <ByteString>value</ByteString>
        if not isinstance(xmlvalue, dom.Element):
            self.value = xmlvalue
            return
        self.checkXML(xmlvalue)
        if xmlvalue.firstChild is None:
            self.value = []  # Catch XML <ByteString /> by setting the value to a default
        else:
            self.value = b64decode(xmlvalue.firstChild.data)

class ExtensionObject(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlelement):
        pass

    def __str__(self):
        return "'ExtensionObject'"

class Structure(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlelement):
        pass

    def __str__(self):
        return "'Structure'"

class LocalizedText(Value):
    def __init__(self, xmlvalue=None):
        Value.__init__(self)
        self.locale = None
        self.text = None
        if xmlvalue:
            self.parseXML(xmlvalue)

    def parseXML(self, xmlvalue):
        # Expect <LocalizedText> or <AliasName>
        #          <Locale>xx_XX</Locale>
        #          <Text>TextText</Text>
        #        <LocalizedText> or </AliasName>
        if not isinstance(xmlvalue, dom.Element):
            self.text = xmlvalue
            return
        self.checkXML(xmlvalue)
        tmp = xmlvalue.getElementsByTagName("Locale")
        if len(tmp) > 0 and tmp[0].firstChild != None:
            self.locale = tmp[0].firstChild.data.strip(' \t\n\r')
        tmp = xmlvalue.getElementsByTagName("Text")
        if len(tmp) > 0 and tmp[0].firstChild != None:
            self.text = tmp[0].firstChild.data.strip(' \t\n\r')

    def __str__(self):
        if self.locale is None and self.text is None:
            return "None"
        if self.locale is not None and len(self.locale) > 0:
            return "(" + self.locale + ":" + self.text + ")"
        else:
            return self.text

    def isNone(self):
        return self.text is None

class NodeId(Value):
    def __init__(self, idstring=None):
        Value.__init__(self)
        self.i = None
        self.b = None
        self.g = None
        self.s = None
        self.ns = 0
        self.setFromIdString(idstring)

    def setFromIdString(self, idstring):
        global namespaceMapping

        if not idstring:
            self.i = 0
            return

        # The ID will encoding itself appropriatly as string. If multiple ID's
        # (numeric, string, guid) are defined, the order of preference for the ID
        # string is always numeric, guid, bytestring, string. Binary encoding only
        # applies to numeric values (UInt16).
        idparts = idstring.strip().split(";")
        for p in idparts:
            if p[:2] == "ns":
                self.ns = int(p[3:])
                if(len(namespaceMapping.values()) > 0):
                    self.ns = namespaceMapping[self.ns]
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
                raise Exception("no valid nodeid: " + idstring)

    # The parsing can be called with an optional namespace mapping dict.
    def parseXML(self, xmlvalue):
        # Expect <NodeId> or <Alias>
        #           <Identifier> # It is unclear whether or not this is manditory. Identifier tags are used in Namespace 0.
        #                ns=x;i=y or similar string representation of id()
        #           </Identifier>
        #        </NodeId> or </Alias>
        if not isinstance(xmlvalue, dom.Element):
            self.text = xmlvalue # Alias
            return
        self.checkXML(xmlvalue)

        # Catch XML <NodeId />
        if xmlvalue.firstChild is None:
            logger.error("No value is given, which is illegal for Node Types...")
            self.value = None
        else:
            # Check if there is an <Identifier> tag
            if len(xmlvalue.getElementsByTagName("Identifier")) != 0:
                xmlvalue = xmlvalue.getElementsByTagName("Identifier")[0]
            self.setFromIdString(unicode(xmlvalue.firstChild.data))

    def __str__(self):
        s = "ns=" + str(self.ns) + ";"
        # Order of preference is numeric, guid, bytestring, string
        if self.i != None:
            return s + "i=" + str(self.i)
        elif self.g != None:
            s = s + "g="
            tmp = []
            for i in self.g:
                tmp.append(hex(i).replace("0x", ""))
            for i in tmp:
                s = s + "-" + i
            return s.replace("g=-", "g=")
        elif self.b != None:
            return s + "b=" + str(self.b)
        elif self.s != None:
            return s + "s=" + str(self.s)

    def isNone(self):
        return self.i is None and self.b is None and self.s is None and self.g is None

    def __eq__(self, nodeId2):
        return (str(self) == str(nodeId2))

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        return str(self)

    def __hash__(self):
        return hash(str(self))

class ExpandedNodeId(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        self.checkXML(xmlvalue)
        logger.debug("Not implemented", LOG_LEVEL_ERR)

class DateTime(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        # Expect <DateTime> or <AliasName>
        #        2013-08-13T21:00:05.0000L
        #        </DateTime> or </AliasName>
        self.checkXML(xmlvalue)
        timestr = getXmlTextTrimmed(xmlvalue.firstChild)

        if timestr is None:
            # Catch XML <DateTime /> by setting the value to a default
            self.value = datetime(2001, 1, 1)
        else:
            # .NET tends to create this garbage %Y-%m-%dT%H:%M:%S.0000z
            # strip everything after the "." away for a posix time_struct
            if "." in timestr:
                timestr = timestr[:timestr.index(".")]
            # If the last character is not numeric, remove it
            while len(timestr) > 0 and not timestr[-1] in "0123456789":
                timestr = timestr[:-1]
            try:
                self.value = datetime.strptime(timestr, "%Y-%m-%dT%H:%M:%S")
            except Exception:
                try:
                    self.value = datetime.strptime(timestr, "%Y-%m-%d")
                except Exception:
                    logger.error("Timestring format is illegible. Expected 2001-01-30T21:22:23 or 2001-01-30, but got " + \
                                 timestr + " instead. Time will be defaultet to now()")
                    self.value = datetime(2001, 1, 1)

class QualifiedName(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        self.ns = 0
        self.name = None
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        # Expect <QualifiedName> or <AliasName>
        #           <NamespaceIndex>Int16<NamespaceIndex>
        #           <Name>SomeString<Name>
        #        </QualifiedName> or </AliasName>
        if not isinstance(xmlvalue, dom.Element):
            colonindex = xmlvalue.find(":")
            if colonindex == -1:
                self.name = xmlvalue
            else:
                self.name = xmlvalue[colonindex + 1:]
                self.ns = int(xmlvalue[:colonindex])
            return

        self.checkXML(xmlvalue)
        # Is a namespace index passed?
        if len(xmlvalue.getElementsByTagName("NamespaceIndex")) != 0:
            self.ns = int(xmlvalue.getElementsByTagName("NamespaceIndex")[0].firstChild.data)
        if len(xmlvalue.getElementsByTagName("Name")) != 0:
            self.name = xmlvalue.getElementsByTagName("Name")[0].firstChild.data


    def __str__(self):
        return "ns=" + str(self.ns) + ";" + str(self.name)

    def isNone(self):
        return self.name is None

class StatusCode(UInt32):
    def __init__(self, xmlelement=None):
        UInt32.__init__(self, xmlelement)

class DiagnosticInfo(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        self.checkXML(xmlvalue)
        logger.warn("Not implemented")

class Guid(Value):
    def __init__(self, xmlelement=None):
        Value.__init__(self)
        if xmlelement:
            self.parseXML(xmlelement)

    def parseXML(self, xmlvalue):
        self.checkXML(xmlvalue)

        # Support GUID in format:
        # <Guid>
        #   <String>01234567-89AB-CDEF-ABCD-0123456789AB</String>
        # </Guid>
        if len(xmlvalue.getElementsByTagName("String")) != 0:
            val = getXmlTextTrimmed(xmlvalue.getElementsByTagName("String")[0].firstChild)

        if val is None:
            self.value = ['00000000', '0000', '0000', '0000', '000000000000']  # Catch XML <Guid /> by setting the value to a default
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
                                 unicode(xmlvalue.firstChild.data))
                    self.value = ['00000000', '0000', '0000', '0000', '000000000000']
            if len(tmp) != 5:
                logger.error("Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + \
                             unicode(xmlvalue.firstChild.data))
                self.value = ['00000000', '0000', '0000', '0000', '000000000000']
