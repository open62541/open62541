#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017, 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH

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

def getXmlTextTrimmed(xmlNode):
    if xmlNode is None or xmlNode.data is None:
        return None
    content = xmlNode.data
    # Check for empty string (including newlines)
    if not re.sub(r"[\s\n\r]", "", content).strip():
        return None
    return content.strip()

#################
# Builtin Types #
#################

class LocalizedText():
    def __init__(self, xmlvalue=None):
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

class NodeId():
    def __init__(self, idstring=None):
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
                if len(namespaceMapping.values()) > 0:
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

    def gAsString(self):
        return '{:08X}-{:04X}-{:04X}-{:04X}-{:012X}'.format(*self.g)

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
            self.setFromIdString(xmlvalue.firstChild.data)

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

class QualifiedName():
    def __init__(self, xmlelement=None):
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
