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
from time import strftime, strptime
import logging; logger = logging.getLogger(__name__)

from constants import *

if sys.version_info[0] >= 3:
  # strings are already parsed to unicode
  def unicode(s):
    return s

knownTypes = ['boolean', 'int32', 'uint32', 'int16', 'uint16', \
              'int64', 'uint64', 'byte', 'sbyte', 'float', 'double', \
              'string', 'bytestring', 'localizedtext', 'statuscode', \
              'diagnosticinfo', 'nodeid', 'guid', 'datetime', \
              'qualifiedname', 'expandednodeid', 'xmlelement']

class Value(object):
  def __init__(self, xmlelement = None):
    self.value = None
    self.numericRepresentation = 0
    self.alias = None
    self.dataType = None
    self.encodingRule = []
    if xmlelement:
      self.parseXML(xmlelement)

  def getValueFieldByAlias(self, fieldname):
    if not isinstance(self.value, list):
        return None
    if not isinstance(self.value[0], Value):
        return None
    for val in self.value:
        if val.alias() == fieldname:
            return val.value
    return None

  def isBuiltinByString(self, string):
    return (str(string).lower() in knownTypes)

  def checkXML(self, xmlvalue):
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      logger.error("Expected XML Element, but got junk...")
      return
    if not self.__class__.__name__ == xmlvalue.tagName:
      logger.warn("Expected XML field " + self.__class__.__name__ + " but got " + xmlvalue.tagName + \
                  " instead. This is a parsing error of Value.__parseXMLSingleValue(), will try to continue anyway.")

  def parseXML(self, xmlvalue):
    self.checkXML(xmlvalue)
    if not "value" in xmlvalue.tagName.lower():
      logger.error("Expected <Value> , but found " + xmlvalue.tagName + " instead. Value will not be parsed.")
      return

    if len(xmlvalue.childNodes) == 0:
      logger.error("Expected childnodes for value, but none where found... Value will not be parsed.")
      return

    for n in xmlvalue.childNodes:
      if n.nodeType == n.ELEMENT_NODE:
        xmlvalue = n
        break

    # if "ListOf" in xmlvalue.tagName:
    #   self.value = []
    #   for el in xmlvalue.childNodes:
    #     if not el.nodeType == el.ELEMENT_NODE:
    #       continue
    #     self.value.append(self.__parseXMLSingleValue(el))
    # else:
    #   self.value = [self.__parseXMLSingleValue(xmlvalue)]

    logger.debug( "Parsed Value: " + str(self.value))

  def __str__(self):
    if self.__alias__ != None:
      return "'" + self.alias() + "':" + self.__class__.__name__ + "(" + str(self.value) + ")"
    return self.__class__.__name__ + "(" + str(self.value) + ")"

  def __repr__(self):
    return self.__str__()

########################
# Actual builtin types #
########################

class Boolean(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_BOOLEAN
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <Boolean>value</Boolean> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = "false" # Catch XML <Boolean /> by setting the value to a default
    else:
      if "false" in unicode(xmlvalue.firstChild.data).lower():
        self.value = "false"
      else:
        self.value = "true"

class Byte(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_BYTE
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <Byte>value</Byte> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Byte /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class SByte(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_SBYTE
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <SByte>value</SByte> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <SByte /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Int16(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_INT16
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <Int16>value</Int16> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Int16 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class UInt16(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_UINT16
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <UInt16>value</UInt16> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <UInt16 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Int32(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_INT32
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <Int32>value</Int32> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Int32 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class UInt32(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_UINT32
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <UInt32>value</UInt32> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <UInt32 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Int64(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_INT64
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <Int64>value</Int64> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Int64 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class UInt64(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_UINT64
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <UInt16>value</UInt16> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <UInt64 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Float(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_FLOAT
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <Float>value</Float> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0.0 # Catch XML <Float /> by setting the value to a default
    else:
      self.value = float(unicode(xmlvalue.firstChild.data))

class Double(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_DOUBLE
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <Double>value</Double> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0.0 # Catch XML <Double /> by setting the value to a default
    else:
      self.value = float(unicode(xmlvalue.firstChild.data))

class String(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_STRING
    if xmlelement:
      self.parseXML(xmlelement)

  def pack(self):
    bin = structpack("I", len(unicode(self.value)))
    bin = bin + str(self.value)
    return bin

  def parseXML(self, xmlvalue):
    # Expect <String>value</String> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = "" # Catch XML <String /> by setting the value to a default
    else:
      self.value = str(unicode(xmlvalue.firstChild.data))

class XmlElement(String):
  def __init__(self, xmlelement = None):
    Value.__init__(self, xmlelement)
    self.numericRepresentation = BUILTINTYPE_TYPEID_XMLELEMENT

class ByteString(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self, xmlelement)
    self.numericRepresentation = BUILTINTYPE_TYPEID_BYTESTRING

class ExtensionObject(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_EXTENSIONOBJECT
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlelement):
    pass

  def __str__(self):
    return "'" + self.alias() + "':" + self.stringRepresentation + "(" + str(self.value) + ")"

class LocalizedText(Value):
  def __init__(self, xmlvalue = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_LOCALIZEDTEXT
    self.value = ['en_US', '']
    if xmlvalue:
      self.parseXML(xmlvalue)

  def parseXML(self, xmlvalue):
    # Expect <LocalizedText> or <AliasName>
    #          <Locale>xx_XX</Locale>
    #          <Text>TextText</Text>
    #        <LocalizedText> or </AliasName>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      return

    tmp = xmlvalue.getElementsByTagName("Locale")
    if len(tmp) == 0:
      self.value[0] = 'en_US'
    else:
      if tmp[0].firstChild == None:
        self.value[0] = 'en_US'
      else:
        self.value[0] = tmp[0].firstChild.data
      clean = ""
      for s in self.value[0]:
        if s in "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_":
          clean = clean + s
      self.value[0] = clean

    tmp = xmlvalue.getElementsByTagName("Text")
    if len(tmp) == 0:
      self.value[1] = ''
    else:
      if tmp[0].firstChild == None:
        self.value[1] = ''
      else:
        self.value[1] = tmp[0].firstChild.data

class NodeId(Value):
  def __init__(self, idstring = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_NODEID
    self.i = -1
    self.o = ""
    self.g = ""
    self.s = ""
    self.ns = 0

    if not idstring:
      self.memoizeString()
      return

    # The ID will encoding itself appropriatly as string. If multiple ID's
    # (numeric, string, guid) are defined, the order of preference for the ID
    # string is always numeric, guid, bytestring, string. Binary encoding only
    # applies to numeric values (UInt16).
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
    self.memoizeString()

  def memoizeString(self):
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

  def __eq__(self, nodeId2):
    return (str(self) == str(nodeId2))

  def __repr__(self):
    return str(self)

  def __hash__(self):
    return hash(str(self))

class ExpandedNodeId(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_EXPANDEDNODEID
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    self.checkXML(xmlvalue)
    logger.debug("Not implemented", LOG_LEVEL_ERR)

class DateTime(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_DATETIME
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <DateTime> or <AliasName>
    #        2013-08-13T21:00:05.0000L
    #        </DateTime> or </AliasName>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None :
      # Catch XML <DateTime /> by setting the value to a default
      self.value = strptime(strftime("%Y-%m-%dT%H:%M%S"), "%Y-%m-%dT%H:%M%S")
    else:
      timestr = unicode(xmlvalue.firstChild.data)
      # .NET tends to create this garbage %Y-%m-%dT%H:%M:%S.0000z
      # strip everything after the "." away for a posix time_struct
      if "." in timestr:
        timestr = timestr[:timestr.index(".")]
      # If the last character is not numeric, remove it
      while len(timestr)>0 and not timestr[-1] in "0123456789":
        timestr = timestr[:-1]
      try:
        self.value = strptime(timestr, "%Y-%m-%dT%H:%M:%S")
      except:
        logger.error("Timestring format is illegible. Expected 2001-01-30T21:22:23, but got " + \
                     timestr + " instead. Time will be defaultet to now()")
        self.value = strptime(strftime("%Y-%m-%dT%H:%M%S"), "%Y-%m-%dT%H:%M%S")

class QualifiedName(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_QUALIFIEDNAME
    self.value = [0, '']
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    # Expect <QualifiedName> or <AliasName>
    #           <NamespaceIndex>Int16<NamespaceIndex> # Optional, apparently ommitted if ns=0?
    #           <Name>SomeString<Name>                # Mandatory
    #        </QualifiedName> or </AliasName>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None :
      self.value = [0, ''] # Catch XML <Qalified /> by setting the value to a default
    else:
      # Is a namespace index passed?
      if len(xmlvalue.getElementsByTagName("NamespaceIndex")) != 0:
        self.value = [int(xmlvalue.getElementsByTagName("NamespaceIndex")[0].firstChild.data)]
        # namespace index is passed and <Name> tags are now manditory?
        if len(xmlvalue.getElementsByTagName("Name")) != 0:
          self.value.append(xmlvalue.getElementsByTagName("Name")[0].firstChild.data)
        else:
          logger.debug("No name is specified, will default to empty string")
          self.value.append('')
      else:
        logger.debug("No namespace is specified, will default to 0")
        self.value = [0]
        self.value.append(unicode(xmlvalue.firstChild.data))

class StatusCode(UInt32):
  def __init__(self, xmlelement = None):
    Value.__init__(self, xmlelement)
    self.numericRepresentation = BUILTINTYPE_TYPEID_STATUSCODE

class DiagnosticInfo(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_DIAGNOSTICINFO
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    self.checkXML(xmlvalue)
    logger.warn("Not implemented")

class Guid(Value):
  def __init__(self, xmlelement = None):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_GUID
    if xmlelement:
      self.parseXML(xmlelement)

  def parseXML(self, xmlvalue):
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = [0,0,0,0] # Catch XML <Guid /> by setting the value to a default
    else:
      self.value = unicode(xmlvalue.firstChild.data)
      self.value = self.value.replace("{","")
      self.value = self.value.replace("}","")
      self.value = self.value.split("-")
      tmp = []
      for g in self.value:
        try:
          tmp.append(int("0x"+g, 16))
        except:
          logger.error("Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + \
                       unicode(xmlvalue.firstChild.data))
          tmp = [0,0,0,0,0]
      if len(tmp) != 5:
        logger.error("Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + \
                     unicode(xmlvalue.firstChild.data))
        tmp = [0,0,0,0]
      self.value = tmp
