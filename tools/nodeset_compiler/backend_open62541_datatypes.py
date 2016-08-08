from datatypes import *

def generateBooleanCode(value):
  if value:
    return "true"
  return "false"

def generateStringCode(value):
  return "UA_STRING(\"" + value + "\")"

def generateXmlElementCode(value):
  return "UA_XMLELEMENT(\"" + value + "\")"

def generateByteStringCode(value):
  return "UA_BYTESTRING(\"" + value + "\")"

def generateLocalizedTextCode(value):
  return "UA_LOCALIZEDTEXT(\"" + value.locale + "\", \"" + value.text + "\")"

def generateQualifiedNameCode(value):
  return "UA_QUALIFIEDNAME(" + str(value.ns) + ", \"" + value.name + "\")"

def generateNodeIdCode(value):
  if not value:
    return "UA_NODEID_NUMERIC(0,0)"
  if value.i != None:
    return "UA_NODEID_NUMERIC(%s,%s)" % (value.ns, value.i)
  elif value.s != None:
    return "UA_NODEID_STRING(%s,%s)" % (value.ns, value.s)
  raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateExpandedNodeIdCode(value):
    if value.i != None:
      return "UA_EXPANDEDVALUE_NUMERIC(%s, %s)" % (str(value.ns),str(value.i))
    elif value.s != None:
      return "UA_EXPANDEDVALUE_STRING(%s, %s)" % (str(value.ns), value.s)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateVariantCode(self):
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
