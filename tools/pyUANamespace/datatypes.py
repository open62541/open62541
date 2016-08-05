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

def getNextElementNode(xmlvalue):
  if xmlvalue == None:
    return None
  xmlvalue = xmlvalue.nextSibling
  while not xmlvalue == None and not xmlvalue.nodeType == xmlvalue.ELEMENT_NODE:
    xmlvalue = xmlvalue.nextSibling
  return xmlvalue

if sys.version_info[0] >= 3:
  # strings are already parsed to unicode
  def unicode(s):
    return s

knownTypes = ['boolean', 'int32', 'uint32', 'int16', 'uint16', \
              'int64', 'uint64', 'byte', 'sbyte', 'float', 'double', \
              'string', 'bytestring', 'localizedtext', 'statuscode', \
              'diagnosticinfo', 'nodeid', 'guid', 'datetime', \
              'qualifiedname', 'expandednodeid', 'xmlelement']

class Value():

  def __init__(self):
    self.value = None
    self.numericRepresentation = 0
    self.alias = None
    self.dataType = None
    self.encodingRule = []

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

  # def getTypeByString(self, stringName, encodingRule):
  #   stringName = str(stringName.lower())
  #   if stringName == 'boolean':
  #     t = Boolean(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'int32':
  #     t = Int32(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'uint32':
  #     t = UInt32(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'int16':
  #     t = Int16(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'uint16':
  #     t = UInt16(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'int64':
  #     t = Int64(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'uint64':
  #     t = UInt64(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'byte':
  #     t = Byte(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'sbyte':
  #     t = SByte(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'float':
  #     t = Float(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'double':
  #     t = Double(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'string':
  #     t = String(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'bytestring':
  #     t = ByteString(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'localizedtext':
  #     t = LocalizedText(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'statuscode':
  #     t = StatusCode(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'diagnosticinfo':
  #     t = DiagnosticInfo(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'nodeid':
  #     t = opcua_BuiltinType_nodeid_t(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'guid':
  #     t = Guid(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'datetime':
  #     t = opcua_BuiltinType_datetime_t(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'qualifiedname':
  #     t = QualifiedName(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'expandednodeid':
  #     t = opcua_BuiltinType_expandednodeid_t(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   elif stringName == 'xmlelement':
  #     t = XmlElement(self.parent)
  #     t.setEncodingRule(encodingRule)
  #   else:
  #     logger.debug("No class representing stringName " + stringName + " was found. Cannot create builtinType.")
  #     return None
  #   return t

  def checkXML(self, xmlvalue):
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      logger.error("Expected XML Element, but got junk...")
      return
    # if self.alias() != None:
    #   if not self.alias() == xmlvalue.tagName:
    #     logger.warn("Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of Value.__parseXMLSingleValue(), will try to continue anyway.")
    # else:
    if not self.__class__.__name__ == xmlvalue.tagName:
      logger.warn("Expected XML field " + self.__class__.__name__ + " but got " + xmlvalue.tagName + " instead. This is a parsing error of Value.__parseXMLSingleValue(), will try to continue anyway.")

  def parseXML(self, xmlvalue):
    logger.debug("parsing xmlvalue for " + self.browseName + " (" + str(self.parent.id) + ") according to " + str(self.parent.dataType.target.getEncoding))

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

    if "ListOf" in xmlvalue.tagName:
      self.value = []
      for el in xmlvalue.childNodes:
        if not el.nodeType == el.ELEMENT_NODE:
          continue
        self.value.append(self.__parseXMLSingleValue(el))
    else:
      self.value = [self.__parseXMLSingleValue(xmlvalue)]

    logger.debug( "Parsed Value: " + str(self.value))

  def __parseXMLSingleValue(self, xmlvalue, alias=None, encodingPart=None):
    # Parse an encoding list such as enc = [[Int32], ['Duration', ['DateTime']]],
    # returning a possibly aliased variable or list of variables.
    # Keep track of aliases, as ['Duration', ['Hawaii', ['UtcTime', ['DateTime']]]]
    # will be of type DateTime, but tagged as <Duration>2013-04-10 12:00 UTC</Duration>,
    # and not as <Duration><Hawaii><UtcTime><String>2013-04-10 12:00 UTC</String>...

    # Encoding may be partially handed down (iterative call). Only resort to
    # type definition if we are not given a specific encoding to match
    if encodingPart == None:
      enc = self.parent.dataType().target().getEncoding()
    else:
      enc = encodingPart

    # Check the structure of the encoding list to determine if a type is to be
    # returned or we need to descend further checking aliases or multipart types
    # such as extension Objects.
    if len(enc) == 1:
      # 0: ['BuiltinType']          either builtin type
      # 1: [ [ 'Alias', [...], n] ] or single alias for possible multipart
      if isinstance(enc[0], str):
        # 0: 'BuiltinType'
        if alias != None:
          if not xmlvalue.tagName == alias:
            logger.error("Expected XML element with tag " + alias + " but found " + xmlvalue.tagName + " instead")
            return None
          else:
            t = self.getTypeByString(enc[0], enc)
            t.alias(alias)
            t.parseXML(xmlvalue)
            return t
        else:
          if not self.isBuiltinByString(xmlvalue.tagName):
            logger.error("Expected XML describing builtin type " + enc[0] + " but found " + xmlvalue.tagName + " instead")
          else:
            t = self.getTypeByString(enc[0], enc)
            t.parseXML(xmlvalue)
            return t
      else:
        # 1: ['Alias', [...], n]
        # Let the next elif handle this
        return self.__parseXMLSingleValue(xmlvalue, alias=alias, encodingPart=enc[0])
    elif len(enc) == 3 and isinstance(enc[0], str):
      # [ 'Alias', [...], 0 ]          aliased multipart
      if alias == None:
        alias = enc[0]
      # if we have an alias and the next field is multipart, keep the alias
      elif alias != None and len(enc[1]) > 1:
        alias = enc[0]
      # otherwise drop the alias
      return self.__parseXMLSingleValue(xmlvalue, alias=alias, encodingPart=enc[1])
    else:
      # [ [...], [...], [...]] multifield of unknowns (analyse separately)
      # create an extension object to hold multipart type

      # FIXME: This implementation expects an extensionobject to be manditory for
      #        multipart variables. Variants/Structures are not included in the
      #        OPCUA Namespace 0 nodeset.
      #        Consider moving this ExtensionObject specific parsing into the
      #        builtin type and only determining the multipart type at this stage.
      if not xmlvalue.tagName == "ExtensionObject":
        logger.error("Expected XML tag <ExtensionObject> for multipart type, but found " + xmlvalue.tagName + " instead.")
        return None

      extobj = opcua_BuiltinType_extensionObject_t(self.parent)
      extobj.setEncodingRule(enc)
      etype = xmlvalue.getElementsByTagName("TypeId")
      if len(etype) == 0:
        logger.error("Did not find <TypeId> for ExtensionObject")
        return None
      etype = etype[0].getElementsByTagName("Identifier")
      if len(etype) == 0:
        logger.error("Did not find <Identifier> for ExtensionObject")
        return None
      etype = self.parent.getNamespace().getNodeByIDString(etype[0].firstChild.data)
      if etype == None:
        logger.error("Identifier Node not found in namespace" )
        return None

      extobj.typeId(etype)

      ebody = xmlvalue.getElementsByTagName("Body")
      if len(ebody) == 0:
        logger.error("Did not find <Body> for ExtensionObject")
        return None
      ebody = ebody[0]

      # Body must contain an Object of type 'DataType' as defined in Variable
      ebodypart = ebody.firstChild
      if not ebodypart.nodeType == ebodypart.ELEMENT_NODE:
        ebodypart = getNextElementNode(ebodypart)
      if ebodypart == None:
        logger.error("Expected ExtensionObject to hold a variable of type " + str(self.parent.dataType().target().browseName()) + " but found nothing.")
        return None

      if not ebodypart.tagName == self.parent.dataType().target().browseName():
        logger.error("Expected ExtensionObject to hold a variable of type " + str(self.parent.dataType().target().browseName()) + " but found " + str(ebodypart.tagName) + " instead.")
        return None
      extobj.alias(ebodypart.tagName)

      ebodypart = ebodypart.firstChild
      if not ebodypart.nodeType == ebodypart.ELEMENT_NODE:
        ebodypart = getNextElementNode(ebodypart)
      if ebodypart == None:
        logger.error("Description of dataType " + str(self.parent.dataType().target().browseName()) + " in ExtensionObject is empty/invalid.")
        return None

      extobj.value = []
      for e in enc:
        if not ebodypart == None:
          extobj.value.append(extobj.__parseXMLSingleValue(ebodypart, alias=None, encodingPart=e))
        else:
          logger.error("Expected encoding " + str(e) + " but found none in body.")
        ebodypart = getNextElementNode(ebodypart)
      return extobj

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
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_BOOLEAN

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
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_BYTE

  def parseXML(self, xmlvalue):
    # Expect <Byte>value</Byte> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Byte /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class SByte(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_SBYTE

  def parseXML(self, xmlvalue):
    # Expect <SByte>value</SByte> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <SByte /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Int16(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_INT16

  def parseXML(self, xmlvalue):
    # Expect <Int16>value</Int16> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Int16 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class UInt16(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_UINT16

  def parseXML(self, xmlvalue):
    # Expect <UInt16>value</UInt16> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <UInt16 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Int32(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_INT32

  def parseXML(self, xmlvalue):
    # Expect <Int32>value</Int32> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Int32 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class UInt32(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_UINT32

  def parseXML(self, xmlvalue):
    # Expect <UInt32>value</UInt32> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <UInt32 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Int64(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_INT64

  def parseXML(self, xmlvalue):
    # Expect <Int64>value</Int64> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <Int64 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class UInt64(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_UINT64

  def parseXML(self, xmlvalue):
    # Expect <UInt16>value</UInt16> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0 # Catch XML <UInt64 /> by setting the value to a default
    else:
      self.value = int(unicode(xmlvalue.firstChild.data))

class Float(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_FLOAT

  def parseXML(self, xmlvalue):
    # Expect <Float>value</Float> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0.0 # Catch XML <Float /> by setting the value to a default
    else:
      self.value = float(unicode(xmlvalue.firstChild.data))

class Double(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_DOUBLE

  def parseXML(self, xmlvalue):
    # Expect <Double>value</Double> or
    #        <Aliasname>value</Aliasname>
    self.checkXML(xmlvalue)
    if xmlvalue.firstChild == None:
      self.value = 0.0 # Catch XML <Double /> by setting the value to a default
    else:
      self.value = float(unicode(xmlvalue.firstChild.data))

class String(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_STRING

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
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_XMLELEMENT

class ByteString(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_BYTESTRING

class ExtensionObject(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_EXTENSIONOBJECT

  def typeId(self, data=None):
    if not data == None:
      self.__typeId__ = data
    return self.__typeId__

  def getCodeInstanceName(self):
    return self.__codeInstanceName__

  def setCodeInstanceName(self, recursionDepth, arrayIndex):
    self.__inVariableRecursionDepth__ = recursionDepth
    self.__inVariableArrayIndex__ = arrayIndex
    self.__codeInstanceName__ = self.parent.getCodePrintableID() + "_" + str(self.alias()) + "_" + str(arrayIndex) + "_" + str(recursionDepth)
    return self.__codeInstanceName__

  def printOpen62541CCode_SubType(self, asIndirect=True):
    if asIndirect == False:
      return "*" + str(self.getCodeInstanceName())
    return str(self.getCodeInstanceName())

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

  # def printOpen62541CCode_SubType(self, asIndirect=True):
  #   if self.value == None:
  #     return "UA_NODEID_NUMERIC(0,0)"
  #   nodeId = self.value.id()
  #   if nodeId.i != None:
  #     return "UA_NODEID_NUMERIC(" + str(nodeId.ns) + ", " + str(nodeId.i) + ")"
  #   elif nodeId.s != None:
  #     return "UA_NODEID_STRING("  + str(nodeId.ns) + ", " + str(nodeId.s) + ")"
  #   elif nodeId.b != None:
  #     logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
  #     return "UA_NODEID_NUMERIC(0,0)"
  #   elif nodeId.g != None:
  #     logger.debug("NodeID Generation macro for guids has not been implemented.")
  #     return "UA_NODEID_NUMERIC(0,0)"
  #   return "UA_NODEID_NUMERIC(0,0)"

class ExpandedNodeId(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_EXPANDEDNODEID

  def parseXML(self, xmlvalue):
    self.checkXML(xmlvalue)
    logger.debug("Not implemented", LOG_LEVEL_ERR)

class DateTime(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_DATETIME

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
        logger.error("Timestring format is illegible. Expected 2001-01-30T21:22:23, but got " + timestr + " instead. Time will be defaultet to now()")
        self.value = strptime(strftime("%Y-%m-%dT%H:%M%S"), "%Y-%m-%dT%H:%M%S")

class QualifiedName(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_QUALIFIEDNAME

  def parseXML(self, xmlvalue):
    # Expect <QualifiedName> or <AliasName>
    #           <NamespaceIndex>Int16<NamespaceIndex> # Optional, apparently ommitted if ns=0 ??? (Not given in OPCUA Nodeset2)
    #           <Name>SomeString<Name>                # Speculation: Manditory if NamespaceIndex is given, omitted otherwise?
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

class StatusCode(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_STATUSCODE

  def parseXML(self, xmlvalue):
    self.checkXML(xmlvalue)
    logger.warn("Not implemented")

class DiagnosticInfo(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_DIAGNOSTICINFO

  def parseXML(self, xmlvalue):
    self.checkXML(xmlvalue)
    logger.warn("Not implemented")

class Guid(Value):
  def __init__(self):
    Value.__init__(self)
    self.numericRepresentation = BUILTINTYPE_TYPEID_GUID

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
      ok = True
      for g in self.value:
        try:
          tmp.append(int("0x"+g, 16))
        except:
          logger.error("Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + unicode(xmlvalue.firstChild.data))
          self.value = [0,0,0,0,0]
          ok = False
      if len(tmp) != 5:
        logger.error("Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + unicode(xmlvalue.firstChild.data))
        self.value = [0,0,0,0]
        ok = False
      self.value = tmp
