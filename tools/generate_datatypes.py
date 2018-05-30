#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this 
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
from nodeset_compiler.opaque_type_mapping import opaque_type_mapping, get_base_type_for_opaque

types = OrderedDict() # contains types that were already parsed
typedescriptions = {} # contains type nodeids

excluded_types = ["NodeIdType", "InstanceNode", "TypeNode", "Node", "ObjectNode",
                  "ObjectTypeNode", "VariableNode", "VariableTypeNode", "ReferenceTypeNode",
                  "MethodNode", "ViewNode", "DataTypeNode",
                  "NumericRange", "NumericRangeDimensions",
                  "UA_ServerDiagnosticsSummaryDataType", "UA_SamplingIntervalDiagnosticsDataType",
                  "UA_SessionSecurityDiagnosticsDataType", "UA_SubscriptionDiagnosticsDataType",
                  "UA_SessionDiagnosticsDataType"]

builtin_types = ["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                 "Int64", "UInt64", "Float", "Double", "String", "DateTime", "Guid",
                 "ByteString", "XmlElement", "NodeId", "ExpandedNodeId", "StatusCode",
                 "QualifiedName", "LocalizedText", "ExtensionObject", "DataValue",
                 "Variant", "DiagnosticInfo"]

# Some types can be memcpy'd off the binary stream. That's especially important
# for arrays. But we need to check if they contain padding and whether the
# endianness is correct. This dict gives the C-statement that must be true for the
# type to be overlayable. Parsed types are added if they apply.
builtin_overlayable = {"Boolean": "true",
                       "SByte": "true", "Byte": "true",
                       "Int16": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "UInt16": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Int32": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "UInt32": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Int64": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "UInt64": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Float": "UA_BINARY_OVERLAYABLE_FLOAT",
                       "Double": "UA_BINARY_OVERLAYABLE_FLOAT",
                       "DateTime": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "StatusCode": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Guid": "(UA_BINARY_OVERLAYABLE_INTEGER && " + \
                       "offsetof(UA_Guid, data2) == sizeof(UA_UInt32) && " + \
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
    def __init__(self, outname, xml, namespace):
        self.name = xml.get("Name")
        self.ns0 = ("true" if namespace == 0 else "false")
        self.typeIndex = outname.upper() + "_" + self.name.upper()
        self.outname = outname
        self.description = ""
        self.pointerfree = "false"
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
        xmlEncodingId = "0"
        binaryEncodingId = "0"
        if self.name in typedescriptions:
            description = typedescriptions[self.name]
            typeid = "{%s, UA_NODEIDTYPE_NUMERIC, {%s}}" % (description.namespaceid, description.nodeid)
            xmlEncodingId = description.xmlEncodingId
            binaryEncodingId = description.binaryEncodingId
        else:
            typeid = "{0, UA_NODEIDTYPE_NUMERIC, {0}}"
        return "{\n    UA_TYPENAME(\"%s\") /* .typeName */\n" % self.name + \
            "    " + typeid + ", /* .typeId */\n" + \
            "    sizeof(UA_" + self.name + "), /* .memSize */\n" + \
            "    " + self.typeIndex + ", /* .typeIndex */\n" + \
            "    " + str(len(self.members)) + ", /* .membersSize */\n" + \
            "    " + self.builtin + ", /* .builtin */\n" + \
            "    " + self.pointerfree + ", /* .pointerFree */\n" + \
            "    " + self.overlayable + ", /* .overlayable */\n" + \
            "    " + binaryEncodingId + ", /* .binaryEncodingId */\n" + \
            "    %s_members" % self.name + " /* .members */\n}"

    def members_c(self):
        if len(self.members)==0:
            return "#define %s_members NULL" % (self.name)
        members = "static UA_DataTypeMember %s_members[%s] = {" % (self.name, len(self.members))
        before = None
        i = 0
        size = len(self.members)
        for index, member in enumerate(self.members):
            i += 1
            membername = member.name
            if len(membername) > 0:
                membername = member.name[0].upper() + member.name[1:]
            m = "\n{\n    UA_TYPENAME(\"%s\") /* .memberName */\n" % membername
            m += "    %s_%s, /* .memberTypeIndex */\n" % (member.memberType.outname.upper(), member.memberType.name.upper())
            m += "    "
            if not before:
                m += "0,"
            else:
                if member.isArray:
                    m += "offsetof(UA_%s, %sSize)" % (self.name, member.name)
                else:
                    m += "offsetof(UA_%s, %s)" % (self.name, member.name)
                m += " - offsetof(UA_%s, %s)" % (self.name, before.name)
                if before.isArray:
                    m += " - sizeof(void*),"
                else:
                    m += " - sizeof(UA_%s)," % before.memberType.name
            m += " /* .padding */\n"
            m += "    %s, /* .namespaceZero */\n" % member.memberType.ns0
            m += ("    true" if member.isArray else "    false") + " /* .isArray */\n}"
            if i != size:
                m += ","
            members += m
            before = member
        return members + "};"

    def datatype_ptr(self):
        return "&" + self.outname.upper() + "[" + self.outname.upper() + "_" + self.name.upper() + "]"

    def functions_c(self):
        funcs = "static UA_INLINE void\nUA_%s_init(UA_%s *p) {\n    memset(p, 0, sizeof(UA_%s));\n}\n\n" % (self.name, self.name, self.name)
        funcs += "static UA_INLINE UA_%s *\nUA_%s_new(void) {\n    return (UA_%s*)UA_new(%s);\n}\n\n" % (self.name, self.name, self.name, self.datatype_ptr())
        if self.pointerfree == "true":
            funcs += "static UA_INLINE UA_StatusCode\nUA_%s_copy(const UA_%s *src, UA_%s *dst) {\n    *dst = *src;\n    return UA_STATUSCODE_GOOD;\n}\n\n" % (self.name, self.name, self.name)
            funcs += "static UA_INLINE void\nUA_%s_deleteMembers(UA_%s *p) { }\n\n" % (self.name, self.name)
        else:
            funcs += "static UA_INLINE UA_StatusCode\nUA_%s_copy(const UA_%s *src, UA_%s *dst) {\n    return UA_copy(src, dst, %s);\n}\n\n" % (self.name, self.name, self.name, self.datatype_ptr())
            funcs += "static UA_INLINE void\nUA_%s_deleteMembers(UA_%s *p) {\n    UA_deleteMembers(p, %s);\n}\n\n" % (self.name, self.name, self.datatype_ptr())
        funcs += "static UA_INLINE void\nUA_%s_delete(UA_%s *p) {\n    UA_delete(p, %s);\n}" % (self.name, self.name, self.datatype_ptr())
        return funcs

    def encoding_h(self):
        enc = "static UA_INLINE size_t\nUA_%s_calcSizeBinary(const UA_%s *src) {\n    return UA_calcSizeBinary(src, %s);\n}\n"
        enc += "static UA_INLINE UA_StatusCode\nUA_%s_encodeBinary(const UA_%s *src, UA_Byte **bufPos, const UA_Byte *bufEnd) {\n    return UA_encodeBinary(src, %s, bufPos, &bufEnd, NULL, NULL);\n}\n"
        enc += "static UA_INLINE UA_StatusCode\nUA_%s_decodeBinary(const UA_ByteString *src, size_t *offset, UA_%s *dst) {\n    return UA_decodeBinary(src, offset, dst, %s, 0, NULL);\n}"
        return enc % tuple(list(itertools.chain(*itertools.repeat([self.name, self.name, self.datatype_ptr()], 3))))

class BuiltinType(Type):
    def __init__(self, name):
        self.name = name
        self.ns0 = "true"
        self.typeIndex = "UA_TYPES_" + self.name.upper()
        self.outname = "ua_types"
        self.description = ""
        self.pointerfree = "false"
        if self.name in builtin_overlayable.keys():
            self.pointerfree = "true"
        self.overlayable = "false"
        if name in builtin_overlayable:
            self.overlayable = builtin_overlayable[name]
        self.builtin = "true"
        self.members = [StructMember("", self, False)] # builtin types contain only one member: themselves (drops into the jumptable during processing)

class EnumerationType(Type):
    def __init__(self, outname, xml, namespace):
        Type.__init__(self, outname, xml, namespace)
        self.pointerfree = "true"
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
        return "typedef enum {\n    " + ",\n    ".join(map(lambda kv : "UA_" + self.name.upper() + "_" + kv[0].upper() + \
                                                           " = " + kv[1], values)) + \
               ",\n    __UA_{0}_FORCE32BIT = 0x7fffffff\n".format(self.name.upper()) + "} " + \
               "UA_{0};\nUA_STATIC_ASSERT(sizeof(UA_{0}) == sizeof(UA_Int32), enum_must_be_32bit);".format(self.name)

class OpaqueType(Type):
    def __init__(self, outname, xml, namespace, baseType):
        Type.__init__(self, outname, xml, namespace)
        self.baseType = baseType
        self.members = [StructMember("", types[baseType], False)] # encoded as string

    def typedef_h(self):
        return "typedef UA_" + self.baseType + " UA_%s;" % self.name

class StructType(Type):
    def __init__(self, outname, xml, namespace):
        Type.__init__(self, outname, xml, namespace)
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

        self.pointerfree = "true"
        self.overlayable = "true"
        before = None
        for m in self.members:
            if m.isArray or m.memberType.pointerfree != "true":
                self.pointerfree = "false"
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

def parseTypeDefinitions(outname, xmlDescription, namespace):
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
                types[name] = EnumerationType(outname, typeXml, namespace)
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
                types[name] = OpaqueType(outname, typeXml, namespace, get_base_type_for_opaque(name)['name'])
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
                types[name] = StructType(outname, typeXml, namespace)
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
        self.xmlEncodingId = "0"
        self.binaryEncodingId = "0"

def parseTypeDescriptions(f, namespaceid):
    definitions = {}
    input_str = f.read()
    input_str = input_str.replace('\r','')
    rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))

    delay_init = []

    for index, row in enumerate(rows):
        if len(row) < 3:
            continue
        if row[2] == "Object":
            # Check if node name ends with _Encoding_(DefaultXml|DefaultBinary) and store the node id in the corresponding DataType
            m = re.match('(.*?)_Encoding_Default(Xml|Binary)$',row[0])
            if (m):
                baseType = m.group(1)
                if baseType not in types:
                    continue

                delay_init.append({
                    "baseType": baseType,
                    "encoding": m.group(2),
                    "id": row[1]
                })
            continue
        if row[2] != "DataType":
            continue
        if row[0] == "BaseDataType":
            definitions["Variant"] = TypeDescription(row[0], row[1], namespaceid)
        elif row[0] == "Structure":
            definitions["ExtensionObject"] = TypeDescription(row[0], row[1], namespaceid)
        elif row[0] not in types:
            continue
        else:
            definitions[row[0]] = TypeDescription(row[0], row[1], namespaceid)
    for i in delay_init:
        if i["baseType"] not in definitions:
            raise Exception("Type {} not found in definitions file.".format(i["baseType"]))
        if i["encoding"] == "Xml":
            definitions[i["baseType"]].xmlEncodingId = i["id"]
        else:
            definitions[i["baseType"]].binaryEncodingId = i["id"]
    return definitions

def merge_dicts(*dict_args):
    """
    Given any number of dicts, shallow copy and merge into a new dict,
    precedence goes to key value pairs in latter dicts.
    """
    result = {}
    for dictionary in dict_args:
        result.update(dictionary)
    return result

###############################
# Parse the Command Line Input#
###############################

parser = argparse.ArgumentParser()
parser.add_argument('-c', '--type-csv',
                    metavar="<typeDescriptions>",
                    type=argparse.FileType('r'),
                    dest="type_csv",
                    action='append',
                    default=[],
                    help='csv file with type descriptions')

parser.add_argument('--namespace',
                    type=int,
                    dest="namespace",
                    default=0,
                    help='namespace id of the generated type nodeids (defaults to 0)')

parser.add_argument('-s', '--selected-types',
                    metavar="<selectedTypes>",
                    type=argparse.FileType('r'),
                    dest="selected_types",
                    action='append',
                    default=[],
                    help='file with list of types (among those parsed) to be generated. If not given, all types are generated')

parser.add_argument('--no-builtin',
                    action='store_true',
                    dest="no_builtin",
                    help='Do not generate builtin types')

parser.add_argument('-t', '--type-bsd',
                    metavar="<typeBsds>",
                    type=argparse.FileType('r'),
                    dest="type_bsd",
                    action='append',
                    default=[],
                    help='bsd file with type definitions')

parser.add_argument('outfile',
                    metavar='<outputFile>',
                    help='output file w/o extension')
args = parser.parse_args()

outname = args.outfile.split("/")[-1]
inname = ', '.join(list(map(lambda x:x.name.split("/")[-1], args.type_bsd)))


################
# Create Types #
################

for builtin in builtin_types:
    types[builtin] = BuiltinType(builtin)

for f in args.type_bsd:
    parseTypeDefinitions(outname, f, args.namespace)

typedescriptions = {}
for f in args.type_csv:
    typedescriptions = merge_dicts(typedescriptions, parseTypeDescriptions(f, args.namespace))

# Read the selected data types
selected_types = []
for f in args.selected_types:
    selected_types += list(filter(len, [line.strip() for line in f]))
# Use all types if none are selected
if len(selected_types) == 0:
    selected_types = types.keys()

#############################
# Write out the Definitions #
#############################

fh = open(args.outfile + "_generated.h",'w')
ff = open(args.outfile + "_generated_handling.h",'w')
fe = open(args.outfile + "_generated_encoding_binary.h",'w')
fc = open(args.outfile + "_generated.c",'w')
def printh(string):
    print(string, end='\n', file=fh)
def printf(string):
    print(string, end='\n', file=ff)
def printe(string):
    print(string, end='\n', file=fe)
def printc(string):
    print(string, end='\n', file=fc)

def iter_types(v):
    l = None
    if sys.version_info[0] < 3:
        l = list(v.itervalues())
    else:
        l = list(v.values())
    if len(selected_types) > 0:
        l = list(filter(lambda t: t.name in selected_types, l))
    if args.no_builtin:
        l = list(filter(lambda t: type(t) != BuiltinType, l))
    return l

################
# Print Header #
################

printh('''/* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
 * on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + \
       ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + ''' */

#ifndef ''' + outname.upper() + '''_GENERATED_H_
#define ''' + outname.upper() + '''_GENERATED_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UA_NO_AMALGAMATION
#include "ua_types.h"
''' + ('#include "ua_types_generated.h"\n' if outname != "ua_types" else '') + '''
#else
#include "open62541.h"
#endif

''')

filtered_types = iter_types(types)

printh('''/**
 * Every type is assigned an index in an array containing the type descriptions.
 * These descriptions are used during type handling (copying, deletion,
 * binary encoding, ...). */''')
printh("#define " + outname.upper() + "_COUNT %s" % (str(len(filtered_types))))
printh("extern UA_EXPORT const UA_DataType " + outname.upper() + "[" + outname.upper() + "_COUNT];")

i = 0
for t in filtered_types:
    printh("\n/**\n * " +  t.name)
    printh(" * " + "^" * len(t.name))
    if t.description == "":
        printh(" */")
    else:
        printh(" * " + t.description + " */")
    if type(t) != BuiltinType:
        printh(t.typedef_h() + "\n")
    printh("#define " + outname.upper() + "_" + t.name.upper() + " " + str(i))
    i += 1

printh('''
#ifdef __cplusplus
} // extern "C"
#endif\n
#endif /* %s_GENERATED_H_ */''' % outname.upper())

##################
# Print Handling #
##################

printf('''/* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
 * on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + \
       ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + ''' */

#ifndef ''' + outname.upper() + '''_GENERATED_HANDLING_H_
#define ''' + outname.upper() + '''_GENERATED_HANDLING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "''' + outname + '''_generated.h"

#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
''')

for t in filtered_types:
    printf("\n/* " + t.name + " */")
    printf(t.functions_c())

printf('''
#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
# pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
} // extern "C"
#endif\n
#endif /* %s_GENERATED_HANDLING_H_ */''' % outname.upper())

###########################
# Print Description Array #
###########################

printc('''/* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
 * on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + \
       ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + ''' */

#include "''' + outname + '''_generated.h"''')

for t in filtered_types:
    printc("")
    printc("/* " + t.name + " */")
    printc(t.members_c())

printc("const UA_DataType %s[%s_COUNT] = {" % (outname.upper(), outname.upper()))
for t in filtered_types:
#    printc("")
    printc("/* " + t.name + " */")
    printc(t.datatype_c() + ",")
printc("};\n")

##################
# Print Encoding #
##################

printe('''/* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
 * on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + \
       ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + ''' */

#include "ua_types_encoding_binary.h"
#include "''' + outname + '''_generated.h"''')

for t in filtered_types:
    printe("\n/* " + t.name + " */")
    printe(t.encoding_h())

fh.close()
ff.close()
fc.close()
fe.close()
