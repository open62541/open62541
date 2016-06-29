from __future__ import print_function
import sys
import time
import platform
import getpass
from collections import OrderedDict
import re
import xml.etree.ElementTree as etree
import itertools
import argparse

types = OrderedDict() # contains types that were already parsed
typedescriptions = {} # contains type nodeids

excluded_types = ["NodeIdType", "InstanceNode", "TypeNode", "Node", "ObjectNode",
                  "ObjectTypeNode", "VariableNode", "VariableTypeNode", "ReferenceTypeNode",
                  "MethodNode", "ViewNode", "DataTypeNode",
                  "UA_ServerDiagnosticsSummaryDataType", "UA_SamplingIntervalDiagnosticsDataType",
                  "UA_SessionSecurityDiagnosticsDataType", "UA_SubscriptionDiagnosticsDataType",
                  "UA_SessionDiagnosticsDataType"]

builtin_types = ["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                 "Int64", "UInt64", "Float", "Double", "String", "DateTime", "Guid",
                 "ByteString", "XmlElement", "NodeId", "ExpandedNodeId", "StatusCode",
                 "QualifiedName", "LocalizedText", "ExtensionObject", "DataValue",
                 "Variant", "DiagnosticInfo"]

# If the type does not contain pointers, it can be copied with memcpy
# (internally, not into the protocol message). This dict contains the sizes of
# fixed-size types. Parsed types are added if they apply.
builtin_fixed_size = ["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                      "Int64", "UInt64", "Float", "Double", "DateTime", "Guid", "StatusCode"]

# Some types can be memcpy'd off the binary stream. That's especially important
# for arrays. But we need to check if they contain padding and whether the
# endianness is correct. This dict gives the C-statement that must be true for the
# type to be overlayable. Parsed types are added if they apply.
builtin_overlayable = {"Boolean": "true", "SByte": "true", "Byte": "true",
                       "Int16": "UA_BINARY_OVERLAYABLE_INTEGER", "UInt16": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Int32": "UA_BINARY_OVERLAYABLE_INTEGER", "UInt32": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Int64": "UA_BINARY_OVERLAYABLE_INTEGER", "UInt64": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Float": "UA_BINARY_OVERLAYABLE_FLOAT", "Double": "UA_BINARY_OVERLAYABLE_FLOAT",
                       "DateTime": "UA_BINARY_OVERLAYABLE_INTEGER", "StatusCode": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Guid": "(UA_BINARY_OVERLAYABLE_INTEGER && offsetof(UA_Guid, data2) == sizeof(UA_UInt32) && " + \
                       "offsetof(UA_Guid, data3) == (sizeof(UA_UInt16) + sizeof(UA_UInt32)) && " + \
                       "offsetof(UA_Guid, data4) == (2*sizeof(UA_UInt32)))"}

################
# Type Classes #
################

class StructMember(object):
    def __init__(self, name, memberType, isArray):
        self.name = name
        self.memberType = memberType
        self.isArray = isArray

class Type(object):
    def __init__(self, outname, xml):
        self.name = xml.get("Name")
        self.ns0 = ("true" if outname == "ua_types" else "false")
        self.typeIndex = outname.upper() + "_" + self.name.upper()
        self.outname = outname
        self.description = ""
        self.fixed_size = "false"
        self.overlayable = "false"
        if self.name in builtin_types:
            self.builtin = "true"
        else:
            self.builtin = "false"
        self.members = [StructMember("", self, False)] # Returns one member: itself. Overwritten by some types.
        for child in xml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                self.description = child.text
                break

    def datatype_c(self):
        if self.name in typedescriptions:
            description = typedescriptions[self.name]
            typeid = "{.namespaceIndex = %s, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = %s}" % (description.namespaceid, description.nodeid)
        else:
            typeid = "{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}"
        return "{ .typeId = " + typeid + \
            ",\n  .typeIndex = " + self.typeIndex + \
            ",\n#ifdef UA_ENABLE_TYPENAMES\n  .typeName = \"%s\",\n#endif\n" % self.name + \
            "  .memSize = sizeof(UA_" + self.name + ")" + \
            ",\n  .builtin = " + self.builtin + \
            ",\n  .fixedSize = " + self.fixed_size + \
            ",\n  .overlayable = " + self.overlayable + \
            ",\n  .membersSize = " + str(len(self.members)) + \
            ",\n  .members = %s_members" % self.name + " }"

    def members_c(self):
        members = "static UA_DataTypeMember %s_members[%s] = {" % (self.name, len(self.members))
        before = None
        for index, member in enumerate(self.members):
            m = "\n  { .memberTypeIndex = %s_%s,\n" % (member.memberType.outname.upper(), member.memberType.name.upper())
            m += "#ifdef UA_ENABLE_TYPENAMES\n    .memberName = \"%s\",\n#endif\n" % member.name
            m += "    .namespaceZero = %s,\n" % member.memberType.ns0
            m += "    .padding = "
            if not before:
                m += "0,\n"
            else:
                if member.isArray:
                    m += "offsetof(UA_%s, %sSize)" % (self.name, member.name)
                else:
                    m += "offsetof(UA_%s, %s)" % (self.name, member.name)
                m += " - offsetof(UA_%s, %s)" % (self.name, before.name)
                if before.isArray:
                    m += " - sizeof(void*),\n"
                else:
                    m += " - sizeof(UA_%s),\n" % before.memberType.name
            m += "    .isArray = " + ("true" if member.isArray else "false")
            members += m + "\n  },"
            before = member
        return members + "};"

    def datatype_ptr(self):
        return "&" + self.outname.upper() + "[" + self.outname.upper() + "_" + self.name.upper() + "]"
        
    def functions_c(self):
        funcs = "static UA_INLINE void UA_%s_init(UA_%s *p) { memset(p, 0, sizeof(UA_%s)); }\n" % (self.name, self.name, self.name)
        funcs += "static UA_INLINE UA_%s * UA_%s_new(void) { return (UA_%s*) UA_new(%s); }\n" % (self.name, self.name, self.name, self.datatype_ptr())
        if self.fixed_size == "true":
            funcs += "static UA_INLINE UA_StatusCode UA_%s_copy(const UA_%s *src, UA_%s *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }\n" % (self.name, self.name, self.name)
            funcs += "static UA_INLINE void UA_%s_deleteMembers(UA_%s *p) { }\n" % (self.name, self.name)
        else:
            funcs += "static UA_INLINE UA_StatusCode UA_%s_copy(const UA_%s *src, UA_%s *dst) { return UA_copy(src, dst, %s); }\n" % (self.name, self.name, self.name, self.datatype_ptr())
            funcs += "static UA_INLINE void UA_%s_deleteMembers(UA_%s *p) { UA_deleteMembers(p, %s); }\n" % (self.name, self.name, self.datatype_ptr())
        funcs += "static UA_INLINE void UA_%s_delete(UA_%s *p) { UA_delete(p, %s); }" % (self.name, self.name, self.datatype_ptr())
        return funcs

    def encoding_h(self):
        enc = "static UA_INLINE UA_StatusCode UA_%s_encodeBinary(const UA_%s *src, UA_ByteString *dst, size_t *offset) { return UA_encodeBinary(src, %s, NULL, NULL, dst, offset); }\n"
        enc += "static UA_INLINE UA_StatusCode UA_%s_decodeBinary(const UA_ByteString *src, size_t *offset, UA_%s *dst) { return UA_decodeBinary(src, offset, dst, %s); }"
        return enc % tuple(list(itertools.chain(*itertools.repeat([self.name, self.name, self.datatype_ptr()], 2))))

class BuiltinType(Type):
    def __init__(self, name):
        self.name = name
        self.ns0 = "true"
        self.typeIndex = "UA_TYPES_" + self.name.upper()
        self.outname = "ua_types"
        self.description = ""
        self.fixed_size = "false"
        if self.name in builtin_fixed_size:
            self.fixed_size = "true"
        self.overlayable = "false"
        if name in builtin_overlayable:
            self.overlayable = builtin_overlayable[name]
        self.builtin = "true"
        if self.name == "QualifiedName":
            self.members = [StructMember("namespaceIndex", types["Int16"], False), StructMember("name", types["String"], False)]
        elif self.name in ["String", "ByteString", "XmlElement"]:
            self.members = [StructMember("", types["Byte"], True)]
        else:
            self.members = [StructMember("", self, False)]

class EnumerationType(Type):
    def __init__(self, outname, xml):
        Type.__init__(self, outname, xml)
        self.fixed_size = "true"
        self.overlayable = "UA_BINARY_OVERLAYABLE_INTEGER"
        self.members = [StructMember("", types["Int32"], False)] # encoded as uint32
        self.builtin = "true"
        self.typeIndex = "UA_TYPES_INT32"
        self.elements = OrderedDict()
        for child in xml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedValue":
                self.elements[child.get("Name")] = child.get("Value")
    
    def typedef_h(self):
        if sys.version_info[0] < 3:
            values = self.elements.iteritems()
        else:
            values = self.elements.items()
        return "typedef enum { \n    " + ",\n    ".join(map(lambda kv : "UA_" + self.name.upper() + "_" + kv[0].upper() + \
                                                            " = " + kv[1], values)) + "\n} UA_%s;" % self.name

class OpaqueType(Type):
    def __init__(self, outname, xml):
        Type.__init__(self, outname, xml)
        self.members = [StructMember("", types["ByteString"], False)] # encoded as string

    def typedef_h(self):
        return "typedef UA_ByteString UA_%s;" % self.name

class StructType(Type):
    def __init__(self, outname, xml):
        Type.__init__(self, outname, xml)
        self.members = []
        lengthfields = [] # lengthfields of arrays are not included as members
        for child in xml:
            if child.get("LengthField"):
                lengthfields.append(child.get("LengthField"))
        for child in xml:
            if not child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                continue
            if child.get("Name") in lengthfields:
                continue
            memberName = child.get("Name")
            memberName = memberName[:1].lower() + memberName[1:]
            memberTypeName = child.get("TypeName")
            memberType = types[memberTypeName[memberTypeName.find(":")+1:]]
            isArray = True if child.get("LengthField") else False
            self.members.append(StructMember(memberName, memberType, isArray))

        self.fixed_size = "true"
        self.overlayable = "true"
        before = None
        for m in self.members:
            if m.isArray or m.memberType.fixed_size != "true":
                self.fixed_size = "false"
                self.overlayable = "false"
            else:
                self.overlayable += " && " + m.memberType.overlayable
                if before:
                    self.overlayable += " && offsetof(UA_%s, %s) == (offsetof(UA_%s, %s) + sizeof(UA_%s))" % \
                                        (self.name, m.name, self.name, before.name, before.memberType.name)
            if "false" in self.overlayable:
                self.overlayable = "false"
            before = m

    def typedef_h(self):
        if len(self.members) == 0:
            return "typedef void * UA_%s;" % self.name
        returnstr =  "typedef struct {\n"
        for member in self.members:
            if member.isArray:
                returnstr += "    size_t %sSize;\n" % member.name
                returnstr += "    UA_%s *%s;\n" % (member.memberType.name, member.name)
            else:
                returnstr += "    UA_%s %s;\n" % (member.memberType.name, member.name)
        return returnstr + "} UA_%s;" % self.name

#########################
# Parse Typedefinitions #
#########################

def parseTypeDefinitions(outname, xmlDescription):
    def typeReady(element):
        "Are all member types defined?"
        for child in element:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                childname = child.get("TypeName")
                if childname[childname.find(":")+1:] not in types:
                    return False
        return True

    def skipType(name):
        if name in excluded_types:
            return True
        if "Test" in name: # skip all test types
            return True
        if re.search("NodeId$", name) != None:
            return True
        return False

    snippets = {}
    for typeXml in etree.parse(xmlDescription).getroot():
        if not typeXml.get("Name"):
            continue
        name = typeXml.get("Name")
        snippets[name] = typeXml

    while(len(snippets) > 0):
        for name, typeXml in list(snippets.items()):
            if name in types or skipType(name):
                del snippets[name]
                continue
            if not typeReady(typeXml):
                continue
            if name in builtin_types:
                types[name] = BuiltinType(name)
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedType":
                types[name] = EnumerationType(outname, typeXml)
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
                types[name] = OpaqueType(outname, typeXml)
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
                types[name] = StructType(outname, typeXml)
            else:
                raise Exception("Type not known")
            del snippets[name]

##########################
# Parse TypeDescriptions #
##########################

class TypeDescription(object):
    def __init__(self, name, nodeid, namespaceid):
        self.name = name
        self.nodeid = nodeid
        self.namespaceid = namespaceid

def parseTypeDescriptions(filename, namespaceid):
    definitions = {}
    with open(filename) as f:
        input_str = f.read()
    input_str = input_str.replace('\r','')
    rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))
    for index, row in enumerate(rows):
        if len(row) < 3:
            continue
        if row[2] != "DataType":
            continue
        if row[0] == "BaseDataType":
            definitions["Variant"] = TypeDescription(row[0], row[1], namespaceid)
        elif row[0] == "Structure":
            definitions["ExtensionObject"] = TypeDescription(row[0], row[1], namespaceid)
        elif row[0] not in types:
            continue
        elif type(types[row[0]]) == EnumerationType:
            definitions[row[0]] = TypeDescription(row[0], "6", namespaceid) # enumerations look like int32 on the wire
        else:
            definitions[row[0]] = TypeDescription(row[0], row[1], namespaceid)
    return definitions

###############################
# Parse the Command Line Input#
###############################

parser = argparse.ArgumentParser()
parser.add_argument('--typedescriptions', help='csv file with type descriptions')
parser.add_argument('--namespace', type=int, default=0, help='namespace id of the generated type nodeids (defaults to 0)')
parser.add_argument('--selected_types', help='file with list of types (among those parsed) to be generated')
parser.add_argument('typexml_ns0', help='path/to/Opc.Ua.Types.bsd ...')
parser.add_argument('typexml_additional', nargs='*', help='path/to/Opc.Ua.Types.bsd ...')
parser.add_argument('outfile', help='output file w/o extension')
args = parser.parse_args()

outname = args.outfile.split("/")[-1] 
inname = ', '.join([args.typexml_ns0.split("/")[-1]] + list(map(lambda x:x.split("/")[-1], args.typexml_additional)))

################
# Create Types #
################

for builtin in builtin_types:
    types[builtin] = BuiltinType(builtin)

with open(args.typexml_ns0) as f:
    parseTypeDefinitions("ua_types", f)
for typexml in args.typexml_additional:
    with open(typexml) as f:
        parseTypeDefinitions(outname, f)

typedescriptions = {}
if args.typedescriptions:
    typedescriptions = parseTypeDescriptions(args.typedescriptions, args.namespace)

selected_types = types.keys()
if args.selected_types:
    with open(args.selected_types) as f:
        selected_types = list(filter(len, [line.strip() for line in f]))

#############################
# Write out the Definitions #
#############################

fh = open(args.outfile + "_generated.h",'w')
fe = open(args.outfile + "_generated_encoding_binary.h",'w')
fc = open(args.outfile + "_generated.c",'w')
def printh(string):
    print(string, end='\n', file=fh)
def printe(string):
    print(string, end='\n', file=fe)
def printc(string):
    print(string, end='\n', file=fc)

printh('''/* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
 * on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + \
       ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + ''' */

#ifndef ''' + outname.upper() + '''_GENERATED_H_
#define ''' + outname.upper() + '''_GENERATED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#ifdef UA_INTERNAL
#include "ua_types_encoding_binary.h"
#endif''' + ('\n#include "ua_types_generated.h"\n' if outname != "ua_types" else '') + '''

/**
 * Additional Data Type Definitions
 * ================================
 */
''')

printh("#define " + outname.upper() + "_COUNT %s" % (str(len(selected_types))))
printh("extern UA_EXPORT const UA_DataType " + outname.upper() + "[" + outname.upper() + "_COUNT];")

printc('''/* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
 * on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + \
       ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + ''' */
 
#include "stddef.h"
#include "ua_types.h"
#include "''' + outname + '''_generated.h"''')

printe('''/* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
 * on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + \
       ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + ''' */
 
#include "ua_types_encoding_binary.h"
#include "''' + outname + '''_generated.h"''')

if sys.version_info[0] < 3:
    values = types.itervalues()
else:
    values = types.values()

# Datatype members
for t in values:
    if not t.name in selected_types:
        continue
    printc("")
    printc("/* " + t.name + " */")
    printc(t.members_c())
printc("const UA_DataType %s[%s_COUNT] = {" % (outname.upper(), outname.upper()))

if sys.version_info[0] < 3:
    values = types.itervalues()
else:
    values = types.values()

i = 0
for t in values:
    if not t.name in selected_types:
        continue
    # Header
    printh("\n/**\n * " +  t.name)
    printh(" * " + "-" * len(t.name))
    if t.description == "":
        printh(" */")
    else:
        printh(" * " + t.description + " */")
    if type(t) != BuiltinType:
        printh(t.typedef_h() + "\n")
    printh("#define " + outname.upper() + "_" + t.name.upper() + " " + str(i))
    printh(t.functions_c())
    i += 1
    # Datatype
    printc("")
    printc("/* " + t.name + " */")
    printc(t.datatype_c() + ",")
    # Encoding
    printe("")
    printe("/* " + t.name + " */")
    printe(t.encoding_h())

printh('''
#ifdef __cplusplus
} // extern "C"
#endif\n
#endif /* %s_GENERATED_H_ */''' % outname.upper())

printc("};\n")

fh.close()
fc.close()
fe.close()
