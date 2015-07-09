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

import xml.dom.minidom as dom
from ua_constants import *
from logger import *
from open62541_MacroHelper import open62541_MacroHelper

def getNextElementNode(xmlvalue):
  if xmlvalue == None:
    return None
  xmlvalue = xmlvalue.nextSibling
  while not xmlvalue == None and not xmlvalue.nodeType == xmlvalue.ELEMENT_NODE:
    xmlvalue = xmlvalue.nextSibling
  return xmlvalue


class opcua_value_t():
  value = None
  name = None
  __alias__ = None
  __binTypeId__ = 0
  stringRepresentation = ""
  knownTypes = []
  parent = None

  def __init__(self, parent):
    self.value = None
    self.parent = parent
    self.stringRepresentation = ""
    self.setStringReprentation()
    self.__binTypeId__ = 0
    self.setNumericRepresentation()
    self.__alias__ = None
    self.knownTypes = ['boolean', 'int32', 'uint32', 'int16', 'uint16', \
                       'int64', 'uint64', 'byte', 'sbyte', 'float', 'double', \
                       'string', 'bytestring', 'localizedtext', 'statuscode', \
                       'diagnosticinfo', 'nodeid', 'guid', 'datetime', \
                       'qualifiedname', 'expandednodeid', 'xmlelement']
    self.dataType = None
    self.encodingRule = []

  def setEncodingRule(self, encoding):
    self.encodingRule = encoding

  def getEncodingRule(self):
    return self.encodingRule

  def alias(self, data=None):
    if not data == None:
      self.__alias__ = data
    return self.__alias__

  def isBuiltinByString(self, string):
    if str(string).lower() in self.knownTypes:
      return True
    return False

  def value(self, data=None):
    if not data==None:
      self.__value__ = data
    return self.__value__

  def getTypeByString(self, stringName, encodingRule):
    stringName = str(stringName.lower())
    if stringName == 'boolean':
      t = opcua_BuiltinType_boolean_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'int32':
      t = opcua_BuiltinType_int32_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'uint32':
      t = opcua_BuiltinType_uint32_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'int16':
      t = opcua_BuiltinType_int16_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'uint16':
      t = opcua_BuiltinType_uint16_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'int64':
      t = opcua_BuiltinType_int64_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'uint64':
      t = opcua_BuiltinType_uint64_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'byte':
      t = opcua_BuiltinType_byte_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'sbyte':
      t = opcua_BuiltinType_sbyte_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'float':
      t = opcua_BuiltinType_float_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'double':
      t = opcua_BuiltinType_double_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'string':
      t = opcua_BuiltinType_string_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'bytestring':
      t = opcua_BuiltinType_bytestring_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'localizedtext':
      t = opcua_BuiltinType_localizedtext_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'statuscode':
      t = opcua_BuiltinType_statuscode_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'diagnosticinfo':
      t = opcua_BuiltinType_diagnosticinfo_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'nodeid':
      t = opcua_BuiltinType_nodeid_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'guid':
      t = opcua_BuiltinType_guid_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'datetime':
      t = opcua_BuiltinType_datetime_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'qualifiedname':
      t = opcua_BuiltinType_qualifiedname_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'expandednodeid':
      t = opcua_BuiltinType_expandednodeid_t(self.parent)
      t.setEncodingRule(encodingRule)
    elif stringName == 'xmlelement':
      t = opcua_BuiltinType_xmlelement_t(self.parent)
      t.setEncodingRule(encodingRule)
    else:
      log(self, "No class representing stringName " + stringName + " was found. Cannot create builtinType.")
      return None
    return t

  def parseXML(self, xmlvalue):
    log(self, "parsing xmlvalue for " + self.parent.browseName() + " (" + str(self.parent.id()) + ") according to " + str(self.parent.dataType().target().getEncoding()))

    if not "value" in xmlvalue.tagName.lower():
      log(self, "Expected <Value> , but found " + xmlvalue.tagName + " instead. Value will not be parsed.", LOG_LEVEL_ERROR)
      return

    if len(xmlvalue.childNodes) == 0:
      log(self, "Expected childnodes for value, but none where found... Value will not be parsed.", LOG_LEVEL_ERROR)
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

    log(self, "Parsed Value: " + str(self.value))

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
            log(self, "Expected XML element with tag " + alias + " but found " + xmlvalue.tagName + " instead", LOG_LEVEL_ERROR)
            return None
          else:
            t = self.getTypeByString(enc[0], enc)
            t.alias(alias)
            t.parseXML(xmlvalue)
            return t
        else:
          if not self.isBuiltinByString(xmlvalue.tagName):
            log(self, "Expected XML describing builtin type " + enc[0] + " but found " + xmlvalue.tagName + " instead", LOG_LEVEL_ERROR)
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
        log(self, "Expected XML tag <ExtensionObject> for multipart type, but found " + xmlvalue.tagName + " instead.", LOG_LEVEL_ERROR)
        return None

      extobj = opcua_BuiltinType_extensionObject_t(self.parent)
      extobj.setEncodingRule(enc)
      etype = xmlvalue.getElementsByTagName("TypeId")
      if len(etype) == 0:
        log(self, "Did not find <TypeId> for ExtensionObject", LOG_LEVEL_ERROR)
        return None
      etype = etype[0].getElementsByTagName("Identifier")
      if len(etype) == 0:
        log(self, "Did not find <Identifier> for ExtensionObject", LOG_LEVEL_ERROR)
        return None
      etype = self.parent.getNamespace().getNodeByIDString(etype[0].firstChild.data)
      if etype == None:
        log(self, "Identifier Node not found in namespace" , LOG_LEVEL_ERROR)
        return None

      extobj.typeId(etype)

      ebody = xmlvalue.getElementsByTagName("Body")
      if len(ebody) == 0:
        log(self, "Did not find <Body> for ExtensionObject", LOG_LEVEL_ERROR)
        return None
      ebody = ebody[0]

      # Body must contain an Object of type 'DataType' as defined in Variable
      ebodypart = ebody.firstChild
      if not ebodypart.nodeType == ebodypart.ELEMENT_NODE:
        ebodypart = getNextElementNode(ebodypart)
      if ebodypart == None:
        log(self, "Expected ExtensionObject to hold a variable of type " + str(self.parent.dataType().target().browseName()) + " but found nothing.", LOG_LEVEL_ERROR)
        return None

      if not ebodypart.tagName == self.parent.dataType().target().browseName():
        log(self, "Expected ExtensionObject to hold a variable of type " + str(self.parent.dataType().target().browseName()) + " but found " + str(ebodypart.tagName) + " instead.", LOG_LEVEL_ERROR)
        return None
      extobj.alias(ebodypart.tagName)

      ebodypart = ebodypart.firstChild
      if not ebodypart.nodeType == ebodypart.ELEMENT_NODE:
        ebodypart = getNextElementNode(ebodypart)
      if ebodypart == None:
        log(self, "Description of dataType " + str(self.parent.dataType().target().browseName()) + " in ExtensionObject is empty/invalid.", LOG_LEVEL_ERROR)
        return None

      extobj.value = []
      for e in enc:
        if not ebodypart == None:
          extobj.value.append(extobj.__parseXMLSingleValue(ebodypart, alias=None, encodingPart=e))
        else:
          log(self, "Expected encoding " + str(e) + " but found none in body.", LOG_LEVEL_ERROR)
        ebodypart = getNextElementNode(ebodypart)
      return extobj

  def setStringReprentation(self):
    pass

  def setNumericRepresentation(self):
    pass

  def getNumericRepresentation(self):
    return self.__binTypeId__

  def __str__(self):
    if self.__alias__ != None:
      return "'" + self.alias() + "':" + self.stringRepresentation + "(" + str(self.value) + ")"
    return self.stringRepresentation + "(" + str(self.value) + ")"

  def __repr__(self):
    return self.__str__()

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return ""

  def printOpen62541CCode(self):
    codegen = open62541_MacroHelper()
    code = []
    valueName = self.parent.getCodePrintableID() + "_variant_DataContents"

    # self.value either contains a list of multiple identical BUILTINTYPES, or it
    # contains a single builtintype (which may be a container); choose if we need
    # to create an array or a single variable.
    # Note that some genious defined that there are arrays of size 1, which are
    # distinctly different then a single value, so we need to check that as well
    # Semantics:
    # -3: Scalar or 1-dim
    # -2: Scalar or x-dim | x>0
    # -1: Scalar
    #  0: x-dim | x>0
    #  n: n-dim | n>0
    if (len(self.value) == 0):
      return code
    if not isinstance(self.value[0], opcua_value_t):
      return code
  
    if self.parent.valueRank() != -1 and (self.parent.valueRank() >=0 or (len(self.value) > 1 and (self.parent.valueRank() != -2 or self.parent.valueRank() != -3))):
      # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
      if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_GUID:
        log(self, "Don't know how to print array of GUID in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DATETIME:
        log(self, "Don't know how to print array of DateTime in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DIAGNOSTICINFO:
        log(self, "Don't know how to print array of DiagnosticInfo in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_STATUSCODE:
        log(self, "Don't know how to print array of StatusCode in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      else:
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          for v in self.value:
            log(self, "Building extObj array index " + str(self.value.index(v)))
            code = code + v.printOpen62541CCode_SubType_build(arrayIndex=self.value.index(v))
        code.append("UA_Variant *" + self.parent.getCodePrintableID() + "_variant = UA_Variant_new();")
        code.append(self.parent.getCodePrintableID() + "_variant->type = &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "];")
        code.append("UA_" + self.value[0].stringRepresentation + " " + valueName + "[" + str(len(self.value)) + "];")
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          for v in self.value:
            log(self, "Printing extObj array index " + str(self.value.index(v)))
            code.append(valueName + "[" + str(self.value.index(v)) + "] = " + v.printOpen62541CCode_SubType(asIndirect=False) + ";")
            code.append("UA_free(" + v.printOpen62541CCode_SubType() + ");")
        else:
          for v in self.value:
            code.append(valueName + "[" + str(self.value.index(v)) + "] = " + v.printOpen62541CCode_SubType() + ";")
        code.append("UA_Variant_setArrayCopy(" + self.parent.getCodePrintableID() + "_variant, &" + valueName + ", (UA_Int32) " + str(len(self.value)) + ", &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "]);")
        code.append(self.parent.getCodePrintableID() + "->value.variant = *" + self.parent.getCodePrintableID() + "_variant;")
    else:
      # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
      if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_GUID:
        log(self, "Don't know how to print scalar GUID in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DATETIME:
        log(self, "Don't know how to print scalar DateTime in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DIAGNOSTICINFO:
        log(self, "Don't know how to print scalar DiagnosticInfo in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_STATUSCODE:
        log(self, "Don't know how to print scalar StatusCode in node " + str(self.parent.id()), LOG_LEVEL_WARN)
      else:
        # The following strategy applies to all other types, in particular strings and numerics.
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          code = code + self.value[0].printOpen62541CCode_SubType_build()
        code.append("UA_Variant *" + self.parent.getCodePrintableID() + "_variant = UA_Variant_new();")
        code.append(self.parent.getCodePrintableID() + "_variant->type = &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "];")
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          code.append("UA_" + self.value[0].stringRepresentation + " *" + valueName + " = " + self.value[0].printOpen62541CCode_SubType() + ";")
          code.append("UA_Variant_setScalarCopy(" + self.parent.getCodePrintableID() + "_variant, " + valueName + ", &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "]);")
          #FIXME: There is no membership definition for extensionObjects generated in this function.
          code.append("UA_" + self.value[0].stringRepresentation + "_deleteMembers(" + valueName + ");")
          code.append(self.parent.getCodePrintableID() + "->value.variant = *" + self.parent.getCodePrintableID() + "_variant;")
        else:
          code.append("UA_" + self.value[0].stringRepresentation + " " + valueName + " = " + self.value[0].printOpen62541CCode_SubType() + ";")
          code.append("UA_Variant_setScalarCopy(" + self.parent.getCodePrintableID() + "_variant, &" + valueName + ", &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "]);")
          code.append("UA_" + self.value[0].stringRepresentation + "_deleteMembers(&" + valueName + ");")
          code.append(self.parent.getCodePrintableID() + "->value.variant = *" + self.parent.getCodePrintableID() + "_variant;")
    return code


###
### Actual buitlin types
###

class opcua_BuiltinType_extensionObject_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "ExtensionObject"
    self.__typeId__ = None

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_EXTENSIONOBJECT

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

  def printOpen62541CCode_SubType_build(self, recursionDepth=0, arrayIndex=0):
    code = [""]
    codegen = open62541_MacroHelper();

    log(self, "Building extensionObject for " + str(self.parent.id()))
    log(self, "Value    " + str(self.value))
    log(self, "Encoding " + str(self.getEncodingRule()))

    self.setCodeInstanceName(recursionDepth, arrayIndex)
    # If there are any ExtensionObjects instide this ExtensionObject, we need to
    # generate one-time-structs for them too before we can proceed;
    for subv in self.value:
      if isinstance(subv, list):
        log(self, "ExtensionObject contains an ExtensionObject, which is currently not encodable!", LOG_LEVEL_ERR)

    code.append("struct {")
    for field in self.getEncodingRule():
      ptrSym = ""
      # If this is an Array, this is pointer to its contents with a AliasOfFieldSize entry
      if field[2] != 0:
        code.append("  UA_Int32 " + str(field[0]) + "Size;")
        ptrSym = "*"
      if len(field[1]) == 1:
        code.append("  UA_" + str(field[1][0]) + " " + ptrSym + str(field[0]) + ";")
      else:
        code.append("  UA_ExtensionObject " + " " + ptrSym + str(field[0]) + ";")
    code.append("} " + self.getCodeInstanceName() + "_struct;")

    # Assign data to the struct contents
    # Track the encoding rule definition to detect arrays and/or ExtensionObjects
    encFieldIdx = 0
    for subv in self.value:
      encField = self.getEncodingRule()[encFieldIdx]
      encFieldIdx = encFieldIdx + 1;
      log(self, "Encoding of field " + subv.alias() + " is " + str(subv.getEncodingRule()) + "defined by " + str(encField))
      # Check if this is an array
      if encField[2] == 0:
        code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + " = " + subv.printOpen62541CCode_SubType(asIndirect=False) + ";")
      else:
        if isinstance(subv, list):
          # this is an array
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "Size = " + str(len(subv)) + ";")
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias()+" = (UA_" + subv.stringRepresentation + " *) UA_malloc(sizeof(UA_" + subv.stringRepresentation + ")*"+ str(len(subv))+");")
          log(self, "Encoding included array of " + str(len(subv)) + " values.")
          for subvidx in range(0,len(subv)):
            subvv = subv[subvidx]
            log(self, "  " + str(subvix) + " " + str(subvv))
            code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "[" + str(subvidx) + "] = " + subvv.printOpen62541CCode_SubType(asIndirect=True) + ";")
          code.append("}")
        else:
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "Size = 1;")
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias()+" = (UA_" + subv.stringRepresentation + " *) UA_malloc(sizeof(UA_" + subv.stringRepresentation + "));")
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "[0]  = " + subv.printOpen62541CCode_SubType(asIndirect=True) + ";")


    # Allocate some memory
    code.append("UA_ExtensionObject *" + self.getCodeInstanceName() + " =  UA_ExtensionObject_new();")
    code.append(self.getCodeInstanceName() + "->encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;")
    code.append(self.getCodeInstanceName() + "->typeId = UA_NODEID_NUMERIC(" + str(self.parent.dataType().target().id().ns) + ", " + str(self.parent.dataType().target().id().i) + "+ UA_ENCODINGOFFSET_BINARY);")
    code.append("UA_ByteString_newMembers(&" + self.getCodeInstanceName() + "->body, 65000);" )

    # Encode each value as a bytestring seperately.
    code.append("size_t " + self.getCodeInstanceName() + "_encOffset = 0;" )
    encFieldIdx = 0;
    for subv in self.value:
      encField = self.getEncodingRule()[encFieldIdx]
      encFieldIdx = encFieldIdx + 1;
      if encField[2] == 0:
        code.append("UA_" + subv.stringRepresentation + "_encodeBinary(&" + self.getCodeInstanceName()+"_struct."+subv.alias() + ", &" + self.getCodeInstanceName() + "->body, &" + self.getCodeInstanceName() + "_encOffset);" )
      else:
        if isinstance(subv, list):
          for subvidx in range(0,len(subv)):
            code.append("UA_" + subv.stringRepresentation + "_encodeBinary(&" + self.getCodeInstanceName()+"_struct."+subv.alias() + "[" + str(subvidx) + "], &" + self.getCodeInstanceName() + "->body, &" + self.getCodeInstanceName() + "_encOffset);" )
        else:
          code.append("UA_" + subv.stringRepresentation + "_encodeBinary(&" + self.getCodeInstanceName()+"_struct."+subv.alias() + "[0], &" + self.getCodeInstanceName() + "->body, &" + self.getCodeInstanceName() + "_encOffset);" )

    # Reallocate the memory by swapping the 65k Bytestring for a new one
    code.append(self.getCodeInstanceName() + "->body.length = " + self.getCodeInstanceName() + "_encOffset;");
    code.append("UA_Byte *" + self.getCodeInstanceName() + "_newBody = (UA_Byte *) UA_malloc(" + self.getCodeInstanceName() + "_encOffset );" )
    code.append("memcpy(" + self.getCodeInstanceName() + "_newBody, " + self.getCodeInstanceName() + "->body.data, " + self.getCodeInstanceName() + "_encOffset);" )
    code.append("UA_Byte *" + self.getCodeInstanceName() + "_oldBody = " + self.getCodeInstanceName() + "->body.data;");
    code.append(self.getCodeInstanceName() + "->body.data = " +self.getCodeInstanceName() + "_newBody;")
    code.append("UA_free(" + self.getCodeInstanceName() + "_oldBody);")
    code.append("")
    return code

  def printOpen62541CCode_SubType(self, asIndirect=True):
    if asIndirect == False:
      return "*" + str(self.getCodeInstanceName())
    return str(self.getCodeInstanceName())

  def __str__(self):
    return "'" + self.alias() + "':" + self.stringRepresentation + "(" + str(self.value) + ")"

class opcua_BuiltinType_localizedtext_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "LocalizedText"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_LOCALIZEDTEXT

  def parseXML(self, xmlvalue):
    # Expect <LocalizedText> or <AliasName>
    #          <Locale>xx_XX</Locale>
    #          <Text>TextText</Text>
    #        <LocalizedText> or </AliasName>
    #
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    if xmlvalue.firstChild == None:
      if self.alias() != None:
        log(self, "Neither locale nor text in XML description field " + self.alias() + ". Setting to default ['en_US','']")
      else:
        log(self, "Neither locale nor text in XML description. Setting to default ['en_US','']")
      self.value = ['en_US','']
      return

    self.value = []
    tmp = xmlvalue.getElementsByTagName("Locale")
    if len(tmp) == 0:
      log(self, "Did not find a locale. Setting to en_US per default.", LOG_LEVEL_WARN)
      self.value.append('en_US')
    else:
      if tmp[0].firstChild == None:
        log(self, "Locale tag without contents. Setting to en_US per default.", LOG_LEVEL_WARN)
        self.value.append('en_US')
      else:
        self.value.append(tmp[0].firstChild.data)
      clean = ""
      for s in self.value[0]:
        if s in "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_":
          clean = clean + s
      self.value[0] = clean

    tmp = xmlvalue.getElementsByTagName("Text")
    if len(tmp) == 0:
      log(self, "Did not find a Text. Setting to empty string per default.", LOG_LEVEL_WARN)
      self.value.append('')
    else:
      if tmp[0].firstChild == None:
        log(self, "Text tag without content. Setting to empty string per default.", LOG_LEVEL_WARN)
        self.value.append('')
      else:
        self.value.append(tmp[0].firstChild.data)

  def printOpen62541CCode_SubType(self, asIndirect=True):
      if asIndirect==True:
        code = "UA_LOCALIZEDTEXT_ALLOC(\"" + str(self.value[0]) + "\", \"" + str(self.value[1].encode('utf-8')) + "\")"
      else:
        code = "UA_LOCALIZEDTEXT(\"" + str(self.value[0]) + "\", \"" + str(self.value[1].encode('utf-8')) + "\")"
      return code

class opcua_BuiltinType_expandednodeid_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "ExpandedNodeId"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_EXPANDEDNODEID

  def parseXML(self, xmlvalue):
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    log(self, "Not implemented", LOG_LEVEL_ERR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    #FIXME! This one is definetely broken!
    code = ""
    return code

class opcua_BuiltinType_nodeid_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "NodeId"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_NODEID

  def parseXML(self, xmlvalue):
    # Expect <NodeId> or <Alias>
    #           <Identifier> # It is unclear whether or not this is manditory. Identifier tags are used in Namespace 0.
    #                ns=x;i=y or similar string representation of id()
    #           </Identifier>
    #        </NodeId> or </Alias>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <NodeId />
    if xmlvalue.firstChild == None :
      log(self, "No value is given, which is illegal for Node Types...", LOG_LEVEL_ERROR)
      self.value = None
    else:
      # Check if there is an <Identifier> tag
      if len(xmlvalue.getElementsByTagName("Identifier")) != 0:
        xmlvalue = xmlvalue.getElementsByTagName("Identifier")[0]
      self.value = self.parent.getNamespace().getNodeByIDString(unicode(xmlvalue.firstChild.data))
      if self.value == None:
        log(self, "Node with id " + str(unicode(xmlvalue.firstChild.data)) + " was not found in namespace.", LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    if self.value == None:
      return "UA_NODEID_NUMERIC(0,0)"
    nodeId = self.value.id()
    if nodeId.i != None:
      return "UA_NODEID_NUMERIC(" + str(nodeId.ns) + ", " + str(nodeId.i) + ")"
    elif nodeId.s != None:
      return "UA_NODEID_STRING("  + str(nodeId.ns) + ", " + str(nodeId.s) + ")"
    elif nodeId.b != None:
      log(self, "NodeID Generation macro for bytestrings has not been implemented.")
      return "UA_NODEID_NUMERIC(0,0)"
    elif nodeId.g != None:
      log(self, "NodeID Generation macro for guids has not been implemented.")
      return "UA_NODEID_NUMERIC(0,0)"
    return "UA_NODEID_NUMERIC(0,0)"

class opcua_BuiltinType_datetime_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "DateTime"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_DATETIME

  def parseXML(self, xmlvalue):
    # Expect <DateTime> or <AliasName>
    #        2013-08-13T21:00:05.0000L
    #        </DateTime> or </AliasName>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <DateTime /> by setting the value to a default
    if xmlvalue.firstChild == None :
      log(self, "No value is given. Setting to default now()")
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
        log(self, "Timestring format is illegible. Expected 2001-01-30T21:22:23, but got " + timestr + " instead. Time will be defaultet to now()", LOG_LEVEL_ERROR)
        self.value = strptime(strftime("%Y-%m-%dT%H:%M%S"), "%Y-%m-%dT%H:%M%S")

class opcua_BuiltinType_qualifiedname_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "QualifiedName"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_QUALIFIEDNAME

  def parseXML(self, xmlvalue):
    # Expect <QualifiedName> or <AliasName>
    #           <NamespaceIndex>Int16<NamespaceIndex> # Optional, apparently ommitted if ns=0 ??? (Not given in OPCUA Nodeset2)
    #           <Name>SomeString<Name>                # Speculation: Manditory if NamespaceIndex is given, omitted otherwise?
    #        </QualifiedName> or </AliasName>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Qalified /> by setting the value to a default
    if xmlvalue.firstChild == None :
      log(self, "No value is given. Setting to default empty string in ns=0: [0, '']")
      self.value = [0, '']
    else:
      # Is a namespace index passed?
      if len(xmlvalue.getElementsByTagName("NamespaceIndex")) != 0:
        self.value = [int(xmlvalue.getElementsByTagName("NamespaceIndex")[0].firstChild.data)]
        # namespace index is passed and <Name> tags are now manditory?
        if len(xmlvalue.getElementsByTagName("Name")) != 0:
          self.value.append(xmlvalue.getElementsByTagName("Name")[0].firstChild.data)
        else:
          log(self, "No name is specified, will default to empty string")
          self.value.append('')
      else:
        log(self, "No namespace is specified, will default to 0")
        self.value = [0]
        self.value.append(unicode(xmlvalue.firstChild.data))

  def printOpen62541CCode_SubType(self, asIndirect=True):
      code = "UA_QUALIFIEDNAME_ALLOC(" + str(self.value[0]) + ", \"" + self.value[1].encode('utf-8') + "\")"
      return code

class opcua_BuiltinType_statuscode_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "StatusCode"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_STATUSCODE

  def parseXML(self, xmlvalue):
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return
    log(self, "Not implemented", LOG_LEVEL_WARN)

class opcua_BuiltinType_diagnosticinfo_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "StatusCode"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_DIAGNOSTICINFO

  def parseXML(self, xmlvalue):
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return
    log(self, "Not implemented", LOG_LEVEL_WARN)

class opcua_BuiltinType_guid_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Guid"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_GUID

  def parseXML(self, xmlvalue):
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Guid /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = [0,0,0,0]
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
          log(self, "Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)
          self.value = [0,0,0,0,0]
          ok = False
      if len(tmp) != 5:
        log(self, "Invalid formatting of Guid. Expected {01234567-89AB-CDEF-ABCD-0123456789AB}, got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)
        self.value = [0,0,0,0]
        ok = False
      self.value = tmp

class opcua_BuiltinType_boolean_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Boolean"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_BOOLEAN

  def parseXML(self, xmlvalue):
    # Expect <Boolean>value</Boolean> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Boolean /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = False
    else:
      if "false" in unicode(xmlvalue.firstChild.data).lower():
        self.value = False
      else:
        self.value = True

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_byte_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Byte"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_BYTE

  def parseXML(self, xmlvalue):
    # Expect <Byte>value</Byte> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Byte /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_sbyte_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "SByte"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_SBYTE

  def parseXML(self, xmlvalue):
    # Expect <SByte>value</SByte> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <SByte /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_int16_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Int16"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_INT16

  def parseXML(self, xmlvalue):
    # Expect <Int16>value</Int16> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Int16 /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_uint16_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "UInt16"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_UINT16

  def parseXML(self, xmlvalue):
    # Expect <UInt16>value</UInt16> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <UInt16 /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_int32_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Int32"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_INT32

  def parseXML(self, xmlvalue):
    # Expect <Int32>value</Int32> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Int32 /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_uint32_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "UInt32"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_UINT32

  def parseXML(self, xmlvalue):
    # Expect <UInt32>value</UInt32> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <UInt32 /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_int64_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Int64"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_INT64

  def parseXML(self, xmlvalue):
    # Expect <Int64>value</Int64> or
    #        <Aliasname>value</Aliasname>
    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Int64 /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_uint64_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "UInt64"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_UINT64

  def parseXML(self, xmlvalue):
    # Expect <UInt16>value</UInt16> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <UInt64 /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0
    else:
      try:
        self.value = int(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_float_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Float"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_FLOAT

  def parseXML(self, xmlvalue):
    # Expect <Float>value</Float> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Float /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0.0
    else:
      try:
        self.value = float(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_double_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "Double"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_DOUBLE

  def parseXML(self, xmlvalue):
    # Expect <Double>value</Double> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <Double /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = 0.0
    else:
      try:
        self.value = float(unicode(xmlvalue.firstChild.data))
      except:
        log(self, "Error parsing integer. Expected " + self.stringRepresentation + " but got " + unicode(xmlvalue.firstChild.data), LOG_LEVEL_ERROR)

  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_string_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "String"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_STRING

  def pack(self):
    bin = structpack("I", len(unicode(self.value)))
    bin = bin + str(self.value)
    return bin

  def parseXML(self, xmlvalue):
    # Expect <String>value</String> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <String /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = ""
    else:
      self.value = str(unicode(xmlvalue.firstChild.data))

  def printOpen62541CCode_SubType(self, asIndirect=True):
      code = "UA_STRING_ALLOC(\"" + self.value.encode('utf-8') + "\")"
      return code

class opcua_BuiltinType_xmlelement_t(opcua_BuiltinType_string_t):
  def setStringReprentation(self):
    self.stringRepresentation = "XmlElement"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_XMLELEMENT

  def printOpen62541CCode_SubType(self, asIndirect=True):
      code = "UA_XMLELEMENT_ALLOC(\"" + self.value.encode('utf-8') + "\")"
      return code

class opcua_BuiltinType_bytestring_t(opcua_value_t):
  def setStringReprentation(self):
    self.stringRepresentation = "ByteString"

  def setNumericRepresentation(self):
    self.__binTypeId__ = BUILTINTYPE_TYPEID_BYTESTRING

  def parseXML(self, xmlvalue):
    # Expect <ByteString>value</ByteString> or
    #        <Aliasname>value</Aliasname>
    if xmlvalue == None or xmlvalue.nodeType != xmlvalue.ELEMENT_NODE:
      log(self, "Expected XML Element, but got junk...", LOG_LEVEL_ERROR)
      return

    if self.alias() != None:
      if not self.alias() == xmlvalue.tagName:
        log(self, "Expected an aliased XML field called " + self.alias() + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)
    else:
      if not self.stringRepresentation == xmlvalue.tagName:
        log(self, "Expected XML field " + self.stringRepresentation + " but got " + xmlvalue.tagName + " instead. This is a parsing error of opcua_value_t.__parseXMLSingleValue(), will try to continue anyway.", LOG_LEVEL_WARN)

    # Catch XML <ByteString /> by setting the value to a default
    if xmlvalue.firstChild == None:
      log(self, "No value is given. Setting to default 0")
      self.value = ""
    else:
      self.value = str(unicode(xmlvalue.firstChild.data))

  def printOpen62541CCode_SubType(self, asIndirect=True):
      bs = ""
      for line in self.value:
        bs = bs + str(line).replace("\n","");
      outs = bs
      log(self, "Encoded Bytestring: " + outs, LOG_LEVEL_DEBUG)
#      bs = bs.decode('base64')
#      outs = ""
#      for s in bs:
#        outs = outs + hex(ord(s)).upper().replace("0X", "\\x")
      code = "UA_STRING_ALLOC(\"" + outs + "\")"
      return code
