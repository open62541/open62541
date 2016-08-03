import * from ua_builtin_types
import open62541Backend from open62541_MacroHelper

class opcua_value_t_open62541(opcua_value_t):
  def printOpen62541CCode(self, bootstrapping = True):
    codegen = open62541Backend()
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

    if self.parent.valueRank() != -1 and \
       (self.parent.valueRank() >=0 or (len(self.value) > 1 and \
                                        (self.parent.valueRank() != -2 or self.parent.valueRank() != -3))):
      # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
      if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_GUID:
        logger.warn("Don't know how to print array of GUID in node " + str(self.parent.id()))
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DATETIME:
        logger.warn("Don't know how to print array of DateTime in node " + str(self.parent.id()))
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DIAGNOSTICINFO:
        logger.warn("Don't know how to print array of DiagnosticInfo in node " + str(self.parent.id()))
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_STATUSCODE:
        logger.warn("Don't know how to print array of StatusCode in node " + str(self.parent.id()))
      else:
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          for v in self.value:
            logger.debug("Building extObj array index " + str(self.value.index(v)))
            code.extend(v.printOpen62541CCode_SubType_build(arrayIndex=self.value.index(v)))
        #code.append("attr.value.type = &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "];")
        code.append("UA_" + self.value[0].stringRepresentation + " " + valueName + \
                    "[" + str(len(self.value)) + "];")
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          for v in self.value:
            logger.debug("Printing extObj array index " + str(self.value.index(v)))
            code.append(valueName + "[" + str(self.value.index(v)) + "] = " + \
                        v.printOpen62541CCode_SubType(asIndirect=False) + ";")
            code.append("UA_free(" + v.printOpen62541CCode_SubType() + ");")
        else:
          for v in self.value:
            code.append(valueName + "[" + str(self.value.index(v)) + "] = " + \
                        v.printOpen62541CCode_SubType() + ";")
        code.append("UA_Variant_setArray( &attr.value, &" + valueName +
                    ", (UA_Int32) " + str(len(self.value)) + ", &UA_TYPES[UA_TYPES_" + \
                    self.value[0].stringRepresentation.upper() + "]);")
    else:
      # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
      if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_GUID:
        logger.warn("Don't know how to print scalar GUID in node " + str(self.parent.id()))
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DATETIME:
        logger.warn("Don't know how to print scalar DateTime in node " + str(self.parent.id()))
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_DIAGNOSTICINFO:
        logger.warn("Don't know how to print scalar DiagnosticInfo in node " + str(self.parent.id()))
      elif self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_STATUSCODE:
        logger.warn("Don't know how to print scalar StatusCode in node " + str(self.parent.id()))
      else:
        # The following strategy applies to all other types, in particular strings and numerics.
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          code.extend(self.value[0].printOpen62541CCode_SubType_build())
        #code.append("attr.value.type = &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "];")
        if self.value[0].__binTypeId__ == BUILTINTYPE_TYPEID_EXTENSIONOBJECT:
          code.append("UA_" + self.value[0].stringRepresentation + " *" + valueName + " = " + \
                      self.value[0].printOpen62541CCode_SubType() + ";")
          code.append("UA_Variant_setScalar( &attr.value, " + valueName + ", &UA_TYPES[UA_TYPES_" + \
                      self.value[0].stringRepresentation.upper() + "]);")

          #FIXME: There is no membership definition for extensionObjects generated in this function.
          #code.append("UA_" + self.value[0].stringRepresentation + "_deleteMembers(" + valueName + ");")
        else:
          if bootstrapping == True:
              code.append("UA_Variant* " + self.parent.getCodePrintableID() + "_variant = UA_Variant_new();" )
          code.append("UA_" + self.value[0].stringRepresentation + " *" + valueName + " =  UA_" + \
                      self.value[0].stringRepresentation + "_new();")
          code.append("*" + valueName + " = " + self.value[0].printOpen62541CCode_SubType() + ";")
          if bootstrapping == False:
            code.append("UA_Variant_setScalar( &attr.value, " + valueName + ", &UA_TYPES[UA_TYPES_" + \
                        self.value[0].stringRepresentation.upper() + "]);")
          else:
            code.append("UA_Variant_setScalar( "+self.parent.getCodePrintableID()+"_variant, " + \
                        valueName + ", &UA_TYPES[UA_TYPES_" + self.value[0].stringRepresentation.upper() + "]);")
          #code.append("UA_" + self.value[0].stringRepresentation + "_deleteMembers(" + valueName + ");")
    return code

class opcua_BuiltinType_boolean_t_open62541(opcua_BuiltinType_boolean_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_byte_t_open62541(opcua_BuiltinType_byte_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_sbyte_t_open62541(opcua_BuiltinType_sbyte_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_int16_t_open62541(opcua_BuiltinType_int16_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_uint16_t_open62541(opcua_BuiltinType_uint16_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_int32_t_open62541(opcua_BuiltinType_int32_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_uint32_t_open62541(opcua_BuiltinType_uint32_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_int64_t_open62541(opcua_BuiltinType_int64_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_uint64_t_open62541(opcua_BuiltinType_uint64_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_float_t_open62541(opcua_BuiltinType_float_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_double_t_open62541(opcua_BuiltinType_double_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    return "(UA_" + self.stringRepresentation + ") " + str(self.value)

class opcua_BuiltinType_string_t_open62541(opcua_BuiltinType_string_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
      code = "UA_STRING_ALLOC(\"" + self.value.encode('utf-8') + "\")"
      return code

class opcua_BuiltinType_xmlelement_t_open62541(opcua_BuiltinType_xmlelement_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
      code = "UA_XMLELEMENT_ALLOC(\"" + self.value.encode('utf-8') + "\")"
      return code

class opcua_BuiltinType_bytestring_t_open62541(opcua_BuiltinType_bytestring_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
      bs = ""
      for line in self.value:
        bs = bs + str(line).replace("\n","");
      outs = bs
      logger.debug("Encoded Bytestring: " + outs)
#      bs = bs.decode('base64')
#      outs = ""
#      for s in bs:
#        outs = outs + hex(ord(s)).upper().replace("0X", "\\x")
      code = "UA_STRING_ALLOC(\"" + outs + "\")"
      return code

class opcua_BuiltinType_localizedtext_open62541(opcua_BuiltinType_localizedtext_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
      if asIndirect==True:
        return "UA_LOCALIZEDTEXT_ALLOC(\"" + str(self.value[0]) + "\", \"" + \
            str(self.value[1].encode('utf-8')) + "\")"
      else:
        return "UA_LOCALIZEDTEXT(\"" + str(self.value[0]) + "\", \"" + \
            str(self.value[1].encode('utf-8')) + "\")"

class opcua_BuiltinType_qualifiedname_t_open62541(opcua_BuiltinType_qualifiedname_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
      code = "UA_QUALIFIEDNAME_ALLOC(" + str(self.value[0]) + ", \"" + self.value[1].encode('utf-8') + "\")"
      return code

class opcua_BuiltinType_nodeid_t_open62541(opcua_BuiltinType_nodeid_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    if self.value == None:
      return "UA_NODEID_NUMERIC(0,0)"
    nodeId = self.value.id()
    if nodeId.i != None:
      return "UA_NODEID_NUMERIC(" + str(nodeId.ns) + ", " + str(nodeId.i) + ")"
    elif nodeId.s != None:
      return "UA_NODEID_STRING("  + str(nodeId.ns) + ", " + str(nodeId.s) + ")"
    elif nodeId.b != None:
      logger.debug("NodeID Generation macro for bytestrings has not been implemented.")
      return "UA_NODEID_NUMERIC(0,0)"
    elif nodeId.g != None:
      logger.debug("NodeID Generation macro for guids has not been implemented.")
      return "UA_NODEID_NUMERIC(0,0)"
    return "UA_NODEID_NUMERIC(0,0)"

class opcua_BuiltinType_expandednodeid_t_open62541(opcua_BuiltinType_expandednodeid_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    #FIXME! This one is definetely broken!
    code = ""
    return code

class opcua_BuiltinType_extensionObject_open62541(opcua_BuiltinType_extensionObject_t):
  def printOpen62541CCode_SubType(self, asIndirect=True):
    if asIndirect == False:
      return "*" + str(self.getCodeInstanceName())
    return str(self.getCodeInstanceName())

  def printOpen62541CCode_SubType_build(self, recursionDepth=0, arrayIndex=0):
    code = [""]
    codegen = open62541_MacroHelper();

    logger.debug("Building extensionObject for " + str(self.parent.id()))
    logger.debug("Value    " + str(self.value))
    logger.debug("Encoding " + str(self.getEncodingRule()))

    self.setCodeInstanceName(recursionDepth, arrayIndex)
    # If there are any ExtensionObjects instide this ExtensionObject, we need to
    # generate one-time-structs for them too before we can proceed;
    for subv in self.value:
      if isinstance(subv, list):
        logger.debug("ExtensionObject contains an ExtensionObject, which is currently not encodable!",
                     LOG_LEVEL_ERR)

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
      logger.debug("Encoding of field " + subv.alias() + " is " + str(subv.getEncodingRule()) + \
                   "defined by " + str(encField))
      # Check if this is an array
      if encField[2] == 0:
        code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + " = " + \
                    subv.printOpen62541CCode_SubType(asIndirect=False) + ";")
      else:
        if isinstance(subv, list):
          # this is an array
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "Size = " + str(len(subv)) + ";")
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias()+" = (UA_" + \
                      subv.stringRepresentation + " *) UA_malloc(sizeof(UA_" + subv.stringRepresentation + \
                      ")*"+ str(len(subv))+");")
          logger.debug("Encoding included array of " + str(len(subv)) + " values.")
          for subvidx in range(0,len(subv)):
            subvv = subv[subvidx]
            logger.debug("  " + str(subvix) + " " + str(subvv))
            code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "[" + str(subvidx) + \
                        "] = " + subvv.printOpen62541CCode_SubType(asIndirect=True) + ";")
          code.append("}")
        else:
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "Size = 1;")
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias()+" = (UA_" + \
                      subv.stringRepresentation + " *) UA_malloc(sizeof(UA_" + subv.stringRepresentation + "));")
          code.append(self.getCodeInstanceName()+"_struct."+subv.alias() + "[0]  = " + \
                      subv.printOpen62541CCode_SubType(asIndirect=True) + ";")


    # Allocate some memory
    code.append("UA_ExtensionObject *" + self.getCodeInstanceName() + " =  UA_ExtensionObject_new();")
    code.append(self.getCodeInstanceName() + "->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;")
    code.append(self.getCodeInstanceName() + "->content.encoded.typeId = UA_NODEID_NUMERIC(" + \
                str(self.parent.dataType().target().id().ns) + ", " + \
                str(self.parent.dataType().target().id().i) + "+ UA_ENCODINGOFFSET_BINARY);")
    code.append("if(UA_ByteString_allocBuffer(&" + self.getCodeInstanceName() + \
                "->content.encoded.body, 65000) != UA_STATUSCODE_GOOD) {}" )

    # Encode each value as a bytestring seperately.
    code.append("size_t " + self.getCodeInstanceName() + "_encOffset = 0;" )
    encFieldIdx = 0;
    for subv in self.value:
      encField = self.getEncodingRule()[encFieldIdx]
      encFieldIdx = encFieldIdx + 1;
      if encField[2] == 0:
        code.append("UA_" + subv.stringRepresentation + "_encodeBinary(&" + \
                    self.getCodeInstanceName()+"_struct."+subv.alias() + ", &" + \
                    self.getCodeInstanceName() + "->content.encoded.body, &" + \
                    self.getCodeInstanceName() + "_encOffset);" )
      else:
        if isinstance(subv, list):
          for subvidx in range(0,len(subv)):
            code.append("UA_" + subv.stringRepresentation + "_encodeBinary(&" + \
                        self.getCodeInstanceName()+"_struct."+subv.alias() + \
                        "[" + str(subvidx) + "], &" + self.getCodeInstanceName() + \
                        "->content.encoded.body, &" + self.getCodeInstanceName() + "_encOffset);" )
        else:
          code.append("UA_" + subv.stringRepresentation + "_encodeBinary(&" + \
                      self.getCodeInstanceName()+"_struct."+subv.alias() + "[0], &" + \
                      self.getCodeInstanceName() + "->content.encoded.body, &" + \
                      self.getCodeInstanceName() + "_encOffset);" )

    # Reallocate the memory by swapping the 65k Bytestring for a new one
    code.append(self.getCodeInstanceName() + "->content.encoded.body.length = " + \
                self.getCodeInstanceName() + "_encOffset;");
    code.append("UA_Byte *" + self.getCodeInstanceName() + "_newBody = (UA_Byte *) UA_malloc(" + \
                self.getCodeInstanceName() + "_encOffset );" )
    code.append("memcpy(" + self.getCodeInstanceName() + "_newBody, " + self.getCodeInstanceName() + \
                "->content.encoded.body.data, " + self.getCodeInstanceName() + "_encOffset);" )
    code.append("UA_Byte *" + self.getCodeInstanceName() + "_oldBody = " + \
                self.getCodeInstanceName() + "->content.encoded.body.data;");
    code.append(self.getCodeInstanceName() + "->content.encoded.body.data = " + \
                self.getCodeInstanceName() + "_newBody;")
    code.append("UA_free(" + self.getCodeInstanceName() + "_oldBody);")
    code.append("")
    return code
