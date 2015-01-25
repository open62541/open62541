from __future__ import print_function
import sys
import time
import platform
import getpass
from collections import OrderedDict
import re
from lxml import etree
import argparse

fixed_size = {"UA_Boolean": 1, "UA_SByte": 1, "UA_Byte": 1, "UA_Int16": 2, "UA_UInt16": 2,
              "UA_Int32": 4, "UA_UInt32": 4, "UA_Int64": 8, "UA_UInt64": 8, "UA_Float": 4,
              "UA_Double": 8, "UA_DateTime": 8, "UA_Guid": 16, "UA_StatusCode": 4}

builtin_types = ["UA_Boolean", "UA_Byte", "UA_Int16", "UA_UInt16", "UA_Int32", "UA_UInt32",
                 "UA_Int64", "UA_UInt64", "UA_Float", "UA_Double", "UA_String", "UA_DateTime",
                 "UA_Guid", "UA_ByteString", "UA_XmlElement", "UA_NodeId", "UA_ExpandedNodeId",
                 "UA_StatusCode", "UA_QualifiedName", "UA_LocalizedText", "UA_ExtensionObject",
                 "UA_Variant", "UA_DataValue", "UA_DiagnosticInfo"]

excluded_types = ["UA_NodeIdType", "UA_InstanceNode", "UA_TypeNode", "UA_Node", "UA_ObjectNode",
                  "UA_ObjectTypeNode", "UA_VariableNode", "UA_VariableTypeNode", "UA_ReferenceTypeNode",
                  "UA_MethodNode", "UA_ViewNode", "UA_DataTypeNode", "UA_ServerDiagnosticsSummaryDataType",
                  "UA_SamplingIntervalDiagnosticsDataType", "UA_SessionSecurityDiagnosticsDataType",
                  "UA_SubscriptionDiagnosticsDataType", "UA_SessionDiagnosticsDataType"]

class Type(object):
    def __init__(self, name, description = ""):
        self.name = name
        self.description = description

    def typedef_c(self):
        pass

    def typelayout_c(self):
        pass

class BuiltinType(Type):
    def __init__(self, name, description = "", zeroCopyable = False):
        self.name = name
        self.description = description
        self.zeroCopyable = zeroCopyable

    def fixed_size(self):
        return self.name in fixed_size

    def typelayout_c(self):
        return "{.memSize = sizeof(" + self.name + "), .zeroCopyable = " + \
            ("UA_TRUE" if self.zeroCopyable else "UA_FALSE") + ", .membersSize = 0 }"

class EnumerationType(Type):
    def __init__(self, name, description = "", elements = OrderedDict()):
        self.name = name
        self.description = description
        self.elements = elements # maps a name to an integer value

    def append_enum(name, value):
        self.elements[name] = value

    def fixed_size(self):
        return True

    def typedef_c(self):
        return "typedef enum { \n    " + \
            ",\n    ".join(map(lambda (key,value) : key.upper() + " = " + value,self.elements.iteritems())) + \
            "\n} " + self.name + ";"

    def typelayout_c(self):
        return "{.memSize = sizeof(" + self.name + "), .zeroCopyable = UA_TRUE, " + \
            ".membersSize = 1,\n\t.members[0] = {.memberTypeIndex = UA_INT32," + \
            ".nameSpaceZero = UA_TRUE, .padding = 0, .isArray = UA_FALSE } }"

class OpaqueType(Type):
    def fixed_size(self):
        return False

    def typedef_c(self):
        return "typedef UA_ByteString " + self.name + ";"

    def typelayout_c(self):
        return "{.memSize = sizeof(" + self.name + "), .zeroCopyable = UA_FALSE, " + \
            ".membersSize = 1,\n\t.members[0] = {.memberTypeIndex = UA_BYTE," + \
            ".nameSpaceZero = UA_TRUE, .padding = offsetof(UA_String, data), .isArray = UA_TRUE } }"

class StructMember(object):
    def __init__(self, name, memberType, isArray):
        self.name = name
        self.memberType = memberType
        self.isArray = isArray

class StructType(Type):
    def __init__(self, name, description, members = OrderedDict()):
        self.name = name
        self.description = description
        self.members = members # maps a name to a member definition

    def fixed_size(self):
        if self.name in fixed_size:
            return True
        for m in self.members.values():
            if m.isArray or not m.memberType.fixed_size():
                return False
        return True

    def typedef_c(self):
        if len(self.members) == 0:
            return "typedef void * " + self.name + ";"
        returnstr =  "typedef struct {\n"
        for name, member in self.members.iteritems():
            if member.isArray:
                returnstr += "    UA_Int32 " + name + "Size;\n"
                returnstr += "    " + member.memberType.name + " *" +name + ";\n"
            else:
                returnstr += "    " + member.memberType.name + " " +name + ";\n"
        return returnstr + "} " + self.name + ";"

    def typelayout_c(self):
        layout = "{.memSize = sizeof(" + self.name + "), .zeroCopyable = " + \
                 ("UA_TRUE, " if self.fixed_size() else "UA_FALSE, ") + \
                 ".membersSize = " + str(len(self.members)) + ","
        for index, member in enumerate(self.members.values()):
            layout += "\n\t.members["+ str(index)+ "] = {" + \
                      ".memberTypeIndex = UA_" + member.memberType.name.upper()[3:] + ", " + \
                      ".nameSpaceZero = "+ \
                      ("UA_TRUE, " if args.is_ns0 or member.memberType.name in builtin_types else "UA_FALSE, ") + \
                      ".padding = "
            if index == 0:
                layout += "0, "
            else:
                before = self.members.values()[index-1]
                layout += "offsetof(" + self.name + ", "+ member.name + ") - offsetof(" + self.name + ", " + before.name + ")"
                if member.isArray:
                    layout += " - sizeof(UA_Int32) "
                if before.isArray:
                    layout += " - sizeof(void *), "
                else:
                    layout += " - sizeof(" + before.memberType.name + "), "
            layout += ".isArray = " + ("UA_TRUE" if member.isArray else "UA_FALSE") + " }, "
        return layout + "}"

def parseTypeDefinitions(xmlDescription, type_selection = None):
    '''Returns an ordered dict that maps names to types. The order is such that
       every type depends only on known types. '''
    ns = {"opc": "http://opcfoundation.org/BinarySchema/"}
    tree = etree.parse(xmlDescription)
    typeSnippets = tree.xpath("/opc:TypeDictionary/*[not(self::opc:Import)]", namespaces=ns)
    types = OrderedDict([(t, BuiltinType(t, "", t in fixed_size)) for t in builtin_types])

    # types we do not want to autogenerate
    def skipType(name):
        if name in builtin_types:
            return True
        if name in excluded_types:
            return True
        if "Test" in name: # skip all test types
            return True
        if re.search("NodeId$", name) != None:
            return True
        if type_selection and not(name in type_selection):
            return True
        return False

    def stripTypename(tn):
        return tn[tn.find(":")+1:]

    def camlCase2CCase(item):
        "Member names begin with a lower case character"
        return item[:1].lower() + item[1:] if item else ''

    def typeReady(element):
        "Do we have the member types yet?"
        for child in element:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                if stripTypename(child.get("TypeName")) not in types:
                    return False
        return True

    def parseEnumeration(typeXml):	
        name = "UA_" + typeXml.get("Name")
        description = ""
        elements = OrderedDict()
        for child in typeXml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                description = child.text
            if child.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedValue":
                elements[name + "_" + child.get("Name")] = child.get("Value")
        return EnumerationType(name, description, elements)

    def parseOpaque(typeXml):
        name = "UA_" + typeXml.get("Name")
        description = ""
        for child in typeXml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                description = child.text
        return OpaqueType(name, description)

    def parseStructured(typeXml):
        "Returns None if we miss member descriptions"
        name = "UA_" + typeXml.get("Name")
        description = ""
        for child in typeXml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                description = child.text

        # ignore lengthfields, just tag the array-members as an array
        lengthfields = []
        for child in typeXml:
            if child.get("LengthField"):
                lengthfields.append(child.get("LengthField"))

        members = OrderedDict()
        for child in typeXml:
            if not child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                continue
            if child.get("Name") in lengthfields:
                continue
            memberTypeName = "UA_" + stripTypename(child.get("TypeName"))
            if not memberTypeName in types:
                return None
            memberType = types[memberTypeName]
            memberName = camlCase2CCase(child.get("Name"))
            isArray = True if child.get("LengthField") else False
            members[memberName] = StructMember(memberName, memberType, isArray)

        return StructType(name, description, members)

    finished = False
    while(not finished):
        finished = True
        for typeXml in typeSnippets:
            name = "UA_" + typeXml.get("Name")
            if name in types or skipType(name):
		continue
            if typeXml.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedType":
		t = parseEnumeration(typeXml)
                types[t.name] = t
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
		t = parseOpaque(typeXml)
		types[t.name] = t
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
                t = parseStructured(typeXml)
                if t == None:
                    finished = False
                else:
                    types[t.name] = t
    return types

parser = argparse.ArgumentParser()
parser.add_argument('--is-ns0', action='store_true', help='the builtin-types are added to namespace 0')
parser.add_argument('xml', help='path/to/Opc.Ua.Types.bsd')
parser.add_argument('outfile', help='outfile w/o extension')

args = parser.parse_args()
outname = args.outfile.split("/")[-1] 
inname = args.xml.split("/")[-1]
types = OrderedDict(parseTypeDefinitions(args.xml).items())

fh = open("ua_" + args.outfile + "_generated.h",'w')
fc = open("ua_" + args.outfile + "_generated.c",'w')
def printh(string):
    print(string, end='\n', file=fh)
def printc(string):
    print(string, end='\n', file=fc)

printh('''/**
* @file ''' + outname + '''_generated.h
*
* @brief Autogenerated data types
*
* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
* on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + '''
*/

#ifndef ''' + outname.upper() + '''_GENERATED_H_
#define ''' + outname.upper() + '''_GENERATED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"

/**
* @ingroup types
*
* @defgroup ''' + outname + '''_generated Autogenerated ''' + outname + ''' Types
*
* @brief Data structures that are autogenerated from an XML-Schema.
*
* @{
*/

extern const UA_DataTypeLayout *UA_''' + outname.upper() + ''';
''')

i = 0
for t in types.itervalues():
    if type(t) != BuiltinType:
        printh("")
        if t.description != "":
            printh("/** @brief " + t.description + "*/")
        printh(t.typedef_c())
    printh("#define UA_" + t.name[3:].upper() + " " + str(i))
    i += 1

printh('''
/// @} /* end of group */

#ifdef __cplusplus
} // extern "C"
#endif

#endif''')

printc('''/**
* @file ''' + outname + '''_generated.c
*
* @brief Autogenerated data types
*
* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
* on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + '''
*/

#include "stddef.h"
#include "ua_''' + outname + '''_generated.h"

const UA_DataTypeLayout *UA_TYPES = (const UA_DataTypeLayout[]){''')

for t in types.itervalues():
    if type(t) == BuiltinType and not args.is_ns0:
        continue
    printc("")
    printc("/* " + t.name + "*/")
    printc(t.typelayout_c() + ",")

printc("};")

fh.close()
fc.close()
