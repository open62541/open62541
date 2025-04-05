#!/usr/bin/env python3

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import re
import sys
import copy
from collections import OrderedDict
import argparse

from nodeset_compiler.type_parser import *

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

parser.add_argument('-x', '--xml',
                    metavar="<nodeSetXML>",
                    type=argparse.FileType('rb'),
                    action='append',
                    dest="type_xml",
                    default=[],
                    help='NodeSet XML file.')

parser.add_argument('--namespaceMap',
                    metavar="<namespaceMap>",
                    type=str,
                    dest="namespace_map",
                    action='append',
                    default=["0:http://opcfoundation.org/UA/"],
                    help='Mapping of namespace uri to the resulting namespace index in the server. Default only contains Namespace 0: "0:http://opcfoundation.org/UA/". '
                         'Parameter can be used multiple times to define multiple mappings.')

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

parser.add_argument('--opaque-map',
                    metavar="<opaqueTypeMap>",
                    type=argparse.FileType('r'),
                    dest="opaque_map",
                    action='append',
                    default=[],
                    help='JSON file with opaque type mapping: { \'typename\': { \'ns\': 0,  \'id\': 7, \'name\': \'UInt32\' }, ... }')

parser.add_argument('--internal',
                    action='store_true',
                    dest="internal",
                    help='Given bsd are internal types which do not have any .csv file')

parser.add_argument('--gen-doc',
                    action='store_true',
                    dest="gen_doc",
                    help='Generate a .rst documentation version of the type definition')

parser.add_argument('-t', '--type-bsd',
                    metavar="<typeBsds>",
                    type=argparse.FileType('r'),
                    dest="type_bsd",
                    action='append',
                    default=[],
                    help='bsd file with type definitions')

parser.add_argument('-i', '--import',
                    metavar="<importBsds>",
                    type=str,
                    dest="import_bsd",
                    action='append',
                    default=[],
                    help='combination of TYPE_ARRAY#filepath.bsd with type definitions which should be loaded but not exported/generated')

parser.add_argument('outfile',
                    metavar='<outputFile>',
                    help='output file w/o extension')

###################
# Code Generation #
###################

# Some types can be memcpy'd off the binary stream. That's especially important
# for arrays. But we need to check if they contain padding and whether the
# endianness is correct. This dict gives the C-statement that must be true for the
# type to be overlayable. Parsed types are added to the list if they apply.
#
# Boolean is not overlayable 1-byte type. We get "undefined behavior" errors
# during fuzzing if we don't force the value to either exactly true or false.
builtin_overlayable = {"SByte": "true",
                       "Byte": "true",
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
                       "Guid": "(UA_BINARY_OVERLAYABLE_INTEGER && " +
                               "offsetof(UA_Guid, data2) == sizeof(UA_UInt32) && " +
                               "offsetof(UA_Guid, data3) == (sizeof(UA_UInt16) + sizeof(UA_UInt32)) && " +
                               "offsetof(UA_Guid, data4) == (2*sizeof(UA_UInt32)))"}

whitelistFuncAttrWarnUnusedResult = []  # for instances [ "String", "ByteString", "LocalizedText" ]


# Escape C strings:
def makeCLiteral(value):
    return re.sub(r'(?<!\\)"', r'\\"', value.replace('\\', r'\\\\').replace('\n', r'\\n').replace('\r', r''))

# Strip invalid characters to create valid C identifiers (variable names etc):
def makeCIdentifier(value):
    keywords = frozenset(["double", "int", "float", "char"])
    sanitized = re.sub(r'[^\w]', '', value)
    if sanitized in keywords:
        return "_" + sanitized
    else:
        return sanitized

def getNodeidTypeAndId(nodeId):
    if not nodeId:
        return "UA_NODEIDTYPE_NUMERIC, {0}"
    if '=' not in nodeId:
        return f"UA_NODEIDTYPE_NUMERIC, {{{nodeId}LU}}"
    if nodeId.startswith("i="):
        return f"UA_NODEIDTYPE_NUMERIC, {{{nodeId[2:]}LU}}"
    if nodeId.startswith("s="):
        strId = nodeId[2:]
        return "UA_NODEIDTYPE_STRING, {{ .string = UA_STRING_STATIC(\"{id}\") }}".format(id=strId.replace("\"", "\\\""))

class CGenerator:
    def __init__(self, parser, inname, outfile, is_internal_types, gen_doc, namespaceMap):
        self.parser = parser
        self.inname = inname
        self.outfile = outfile
        self.is_internal_types = is_internal_types
        self.gen_doc = gen_doc
        self.filtered_types = None
        self.namespaceMap = namespaceMap
        self.fh = None
        self.fc = None
        self.fd = None
        self.fe = None

    @staticmethod
    def get_type_index(datatype):
        if isinstance(datatype,  BuiltinType):
            return makeCIdentifier("UA_TYPES_" + datatype.name.upper())
        if isinstance(datatype, EnumerationType):
            return datatype.strTypeIndex

        if datatype.name is not None:
            return "UA_" + makeCIdentifier(datatype.outname.upper() + "_" + datatype.name.upper())
        return makeCIdentifier(datatype.outname.upper())

    @staticmethod
    def get_type_kind(datatype):
        if isinstance(datatype, BuiltinType):
            return "UA_DATATYPEKIND_" + datatype.name.upper()
        if isinstance(datatype, EnumerationType):
            return datatype.strTypeKind
        if isinstance(datatype, OpaqueType):
            return "UA_DATATYPEKIND_" + datatype.base_type.upper()
        if isinstance(datatype, StructType):
            for m in datatype.members:
                if m.is_optional:
                    return "UA_DATATYPEKIND_OPTSTRUCT"
            if datatype.is_union:
                return "UA_DATATYPEKIND_UNION"
            return "UA_DATATYPEKIND_STRUCTURE"
        raise RuntimeError("Unknown type")

    @staticmethod
    def get_struct_overlayable(struct):
        if not struct.pointerfree == "false":
            return "false"
        before = None
        overlayable = ""
        for m in struct.members:
            if m.is_array or not m.member_type.pointerfree:
                return "false"
            overlayable += "\n\t\t && " + m.member_type.overlayable
            if before:
                overlayable += "\n\t\t && offsetof(UA_%s, %s) == (offsetof(UA_%s, %s) + sizeof(UA_%s))" % \
                               (makeCIdentifier(struct.name), makeCIdentifier(m.name), makeCIdentifier(struct.name),
                                makeCIdentifier(before.name), makeCIdentifier(before.member_type.name))
            before = m
        return overlayable

    def get_type_overlayable(self, datatype):
        if isinstance(datatype, BuiltinType) or isinstance(datatype, OpaqueType):
            return builtin_overlayable[datatype.name] if datatype.name in builtin_overlayable else "false"
        if isinstance(datatype, EnumerationType):
            return "UA_BINARY_OVERLAYABLE_INTEGER"
        if isinstance(datatype, StructType):
            return self.get_struct_overlayable(datatype)
        raise RuntimeError("Unknown datatype")

    def print_datatype(self, datatype, namespaceMap):
        typeid = "{{{}, {}}}".format("0", getNodeidTypeAndId(datatype.nodeId))
        binaryEncodingId = "{{{}, {}}}".format("0", getNodeidTypeAndId(datatype.binaryEncodingId))
        xmlEncodingId = "{{{}, {}}}".format("0", getNodeidTypeAndId(datatype.xmlEncodingId))
        idName = makeCIdentifier(datatype.name)
        pointerfree = "true" if datatype.pointerfree else "false"
        return "{\n" + \
               "    UA_TYPENAME(\"%s\") /* .typeName */\n" % idName + \
               "    " + typeid + ", /* .typeId */\n" + \
               "    " + binaryEncodingId + ", /* .binaryEncodingId */\n" + \
               "    " + xmlEncodingId + ", /* .xmlEncodingId */\n" + \
               "    sizeof(UA_" + idName + "), /* .memSize */\n" + \
               "    " + self.get_type_kind(datatype) + ", /* .typeKind */\n" + \
               "    " + pointerfree + ", /* .pointerFree */\n" + \
               "    " + self.get_type_overlayable(datatype) + ", /* .overlayable */\n" + \
               "    " + str(len(datatype.members)) + ", /* .membersSize */\n" + \
               "    %s_members" % idName + "  /* .members */\n" + \
               "}"

    @staticmethod
    def print_members(datatype, namespaceMap):
        idName = makeCIdentifier(datatype.name)
        if len(datatype.members) == 0:
            return "#define %s_members NULL" % (idName)
        isUnion = isinstance(datatype, StructType) and datatype.is_union
        members = "static UA_DataTypeMember {}_members[{}] = {{".format(idName, len(datatype.members))
        before = None
        size = len(datatype.members)
        for i, member in enumerate(datatype.members):

            # Abfrage member_type
            if not member.member_type.members and isinstance(member.member_type, StructType):
                type_name = "ExtensionObject"
            else:
                type_name = member.member_type.name

            if before:
                if not before.member_type.members and isinstance(before.member_type, StructType):
                    type_name_before = "ExtensionObject"
                else:
                    type_name_before = before.member_type.name

            member_name = makeCIdentifier(member.name)
            member_name_capital = member_name
            if len(member_name) > 0:
                member_name_capital = member_name[0].upper() + member_name[1:]
            m = "\n{\n"
            m += "    UA_TYPENAME(\"%s\") /* .memberName */\n" % member_name_capital
            m += "    &UA_{}[UA_{}_{}], /* .memberType */\n".format(
                member.member_type.outname.upper(), member.member_type.outname.upper(),
                makeCIdentifier(type_name.upper()))
            m += "    "
            if not before and not isUnion:
                m += "0,"
            elif isUnion:
                    m += "offsetof(UA_{}, fields.{}),".format(idName, member_name)
            else:
                if member.is_array:
                    m += "offsetof(UA_{}, {}Size)".format(idName, member_name)
                else:
                    m += "offsetof(UA_{}, {})".format(idName, member_name)
                m += " - offsetof(UA_{}, {})".format(idName, makeCIdentifier(before.name))
                if before.is_array or before.is_optional:
                    m += " - sizeof(void *),"
                else:
                    m += " - sizeof(UA_%s)," % makeCIdentifier(type_name_before)
            m += " /* .padding */\n"
            m += ("    true" if member.is_array else "    false") + ", /* .isArray */\n"
            m += ("    true" if member.is_optional else "    false") + "  /* .isOptional */\n}"
            if i != size:
                m += ","
            members += m
            before = member
        return members + "};"

    @staticmethod
    def print_datatype_ptr(datatype):
        return "&UA_" + datatype.outname.upper() + "[UA_" + makeCIdentifier(
            datatype.outname.upper() + "_" + datatype.name.upper()) + "]"

    def print_functions(self, datatype):
        idName = makeCIdentifier(datatype.name)
        funcs = "UA_INLINABLE( void\nUA_{}_init(UA_{} *p), {{\n    memset(p, 0, sizeof(UA_{}));\n}})\n\n".format(idName, idName, idName)
        funcs += "UA_INLINABLE( UA_{} *\nUA_{}_new(void), {{\n    return (UA_{}*)UA_new({});\n}})\n\n".format(idName, idName, idName, CGenerator.print_datatype_ptr(datatype))
        if datatype.pointerfree == "true":
            funcs += "UA_INLINABLE( UA_StatusCode\nUA_{}_copy(const UA_{} *src, UA_{} *dst), {{\n    *dst = *src;\n    return UA_STATUSCODE_GOOD;\n}})\n\n".format(idName, idName, idName)
            funcs += "UA_INLINABLE( void\nUA_{}_clear(UA_{} *p), {{\n    memset(p, 0, sizeof(UA_{}));\n}})\n".format(idName, idName, idName)
        else:
            for entry in whitelistFuncAttrWarnUnusedResult:
                if idName == entry:
                    funcs += "UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT "
                    break

            funcs += "UA_INLINABLE( UA_StatusCode\nUA_{}_copy(const UA_{} *src, UA_{} *dst), {{\n    return UA_copy(src, dst, {});\n}})\n\n".format(idName, idName, idName, self.print_datatype_ptr(datatype))
            funcs += "UA_INLINABLE( void\nUA_{}_clear(UA_{} *p), {{\n    UA_clear(p, {});\n}})\n\n".format(idName, idName, self.print_datatype_ptr(datatype))
        funcs += "UA_INLINABLE( void\nUA_{}_delete(UA_{} *p), {{\n    UA_delete(p, {});\n}})\n\n".format(idName, idName, self.print_datatype_ptr(datatype))
        funcs += "UA_INLINABLE( UA_Boolean\nUA_{}_equal(const UA_{} *p1, const UA_{} *p2), {{\n    return (UA_order(p1, p2, {}) == UA_ORDER_EQ);\n}})\n".format(
            idName, idName, idName, self.print_datatype_ptr(datatype))
        return funcs

    @staticmethod
    def print_enum_typedef(enum, gen_doc=False):
        values = enum.elements.items()
        if enum.isOptionSet == True:
            elements = map(lambda kv: "#define " + makeCIdentifier("UA_" + enum.name.upper() + "_" + kv[0].upper()) + " " + kv[1], values)
            return "typedef " + enum.strDataType + " " + makeCIdentifier("UA_" + enum.name) + ";\n\n" + "\n".join(elements)
        else:
            elements = [makeCIdentifier("UA_" + enum.name.upper() + "_" + kv[0].upper()) + " = " + kv[1] for kv in values]
            if not gen_doc:
                elements.append(f"__UA_{makeCIdentifier(enum.name.upper())}_FORCE32BIT = 0x7fffffff")
            out = []
            out.append("typedef enum {")
            for i,e in enumerate(elements):
                out.append("    " + e)
                if i < len(elements)-1:
                    out[-1] += ","
            out.append(f"}} UA_{makeCIdentifier(enum.name)};")
            if not gen_doc:
                out.append(f"\nUA_STATIC_ASSERT(sizeof(UA_{makeCIdentifier(enum.name)}) == sizeof(UA_Int32), enum_must_be_32bit);")
            return "\n".join(out)

    @staticmethod
    def print_struct_typedef(struct):
        #generate enum option for union
        returnstr = ""
        if struct.is_union:
            #test = type("MyEnumOptionSet", (EnumOptionSet, object), {"foo": lambda self: "foo"})
            obj = type('MyEnumOptionSet', (object,), {'isOptionSet': False, 'elements': OrderedDict(), 'name': struct.name+"Switch"})
            obj.elements['None'] = str(0)
            count = 1
            for member in struct.members:
                obj.elements[member.name] = str(count)
                count += 1
            returnstr += CGenerator.print_enum_typedef(obj)
            returnstr += "\n\n"
        if len(struct.members) == 0:
            raise Exception("Structs with no members are filtered out. Why not here?")
        if struct.is_recursive:
            returnstr += "typedef struct UA_{} UA_{};\n".format(makeCIdentifier(struct.name), makeCIdentifier(struct.name))
            returnstr += "struct UA_%s {\n" % makeCIdentifier(struct.name)
        else:
            returnstr += "typedef struct {\n"
        if struct.is_union:
            returnstr += "    UA_%sSwitch switchField;\n" % struct.name
            returnstr += "    union {\n"
        for member in struct.members:
            if not member.member_type.members and isinstance(member.member_type, StructType):
                type_name = "ExtensionObject"
            else:
                type_name = member.member_type.name

            if member.is_array:
                if struct.is_union:
                    returnstr += "        struct {\n        "
                returnstr += "    size_t %sSize;\n" % makeCIdentifier(member.name)
                if struct.is_union:
                    returnstr += "        "
                returnstr += "    UA_{} *{};\n".format(
                    makeCIdentifier(type_name), makeCIdentifier(member.name))
                if struct.is_union:
                    returnstr += "        } " + makeCIdentifier(member.name) + ";\n"
            elif struct.is_union:
                returnstr += "        UA_{} {};\n".format(
                makeCIdentifier(type_name), makeCIdentifier(member.name))
            elif member.is_optional:
                returnstr += "    UA_{} *{};\n".format(
                    makeCIdentifier(type_name), makeCIdentifier(member.name))
            else:
                returnstr += "    UA_{} {};\n".format(
                    makeCIdentifier(type_name), makeCIdentifier(member.name))
        if struct.is_union:
            returnstr += "    } fields;\n"
        if struct.is_recursive:
            return returnstr + "};"
        else:
            return returnstr + "} UA_%s;" % makeCIdentifier(struct.name)

    @staticmethod
    def print_datatype_typedef(datatype, gen_doc=False):
        if isinstance(datatype, EnumerationType):
            return CGenerator.print_enum_typedef(datatype, gen_doc)
        if isinstance(datatype, OpaqueType):
            return "typedef UA_" + datatype.base_type + " UA_%s;" % datatype.name
        if isinstance(datatype, StructType):
            return CGenerator.print_struct_typedef(datatype)
        raise RuntimeError("Type does not have an associated typedef")

    def write_definitions(self):
        self.fh = open(self.outfile + "_generated.h", 'w')
        self.fc = open(self.outfile + "_generated.c", 'w')

        self.filtered_types = self.iter_types(self.parser.types)

        self.print_header()
        self.print_description_array()

        self.fh.close()
        self.fc.close()

        if self.gen_doc:
            self.fd = open(self.outfile + "_generated.rst", 'w')
            self.print_doc()
            self.fd.close()

    def printh(self, string):
        print(string, end='\n', file=self.fh)

    def printc(self, string):
        print(string, end='\n', file=self.fc)

    def printd(self, string):
        print(string, end='\n', file=self.fd)

    def iter_types(self, v):
        # Make a copy. We cannot delete from the map that is iterated over at
        # the same time.
        l = copy.deepcopy(v)

        # Keep only selected types?
        if len(self.parser.selected_types) > 0:
            for ns in v:
                for t in v[ns]:
                    if t not in self.parser.selected_types:
                        if ns in l and t in l[ns]:
                            del l[ns][t]

        # Remove builtins?
        if self.parser.no_builtin:
            for ns in v:
                for t in v[ns]:
                    if isinstance(v[ns][t], BuiltinType):
                        if ns in l and t in l[ns]:
                            del l[ns][t]

        # Remove types that from other bsd files
        for ns in self.parser.existing_types:
            for t in self.parser.existing_types[ns]:
                if ns in l and t in l[ns]:
                    del l[ns][t]

        # Remove structs with no members
        for ns in v:
            for t in v[ns]:
                if isinstance(v[ns][t], StructType) and len(v[ns][t].members) == 0:
                    if ns in l and t in l[ns]:
                        del l[ns][t]
        return l

    def print_header(self):
        additionalHeaders = ""
        for arr in self.parser.existing_types_array:
            if arr == "UA_TYPES":
                continue
            # remove ua_ prefix if exists
            typeFile = arr.lower()
            typeFile = typeFile[typeFile.startswith("ua_") and len("ua_"):]
            additionalHeaders += """#include "%s_generated.h"\n""" % typeFile

        self.printh('''/**********************************
 * Autogenerated -- do not modify *
 **********************************/

#include <open62541/types.h>

#ifndef ''' + self.parser.outname.upper() + '''_GENERATED_H_
#define ''' + self.parser.outname.upper() + '''_GENERATED_H_

''' + (additionalHeaders) + '''
_UA_BEGIN_DECLS
''')

        self.printh('''/**
 * Every type is assigned an index in an array containing the type descriptions.
 * These descriptions are used during type handling (copying, deletion,
 * binary encoding, ...). */''')
        totalCount = 0
        for ns in self.filtered_types:
            totalCount += len(self.filtered_types[ns])
        self.printh("#define UA_" + self.parser.outname.upper() + "_COUNT %s" % (str(totalCount)))

        if totalCount > 0:

            self.printh(
                "extern UA_EXPORT UA_DataType UA_" + self.parser.outname.upper() + "[UA_" + self.parser.outname.upper() + "_COUNT];")

            for ns in self.filtered_types:
                for i, t_name in enumerate(self.filtered_types[ns]):
                    t = self.filtered_types[ns][t_name]
                    if t.description == "":
                        self.printh("\n/* " + t.name + " */")
                    else:
                        self.printh("\n/* " + t.name + ": " + t.description + " */")
                    if not isinstance(t, BuiltinType):
                        self.printh(self.print_datatype_typedef(t) + "\n")
                    self.printh("#define UA_" + makeCIdentifier(self.parser.outname.upper() + "_" + t.name.upper()) + " " + str(i))
                    self.printh("")
                    self.printh(self.print_functions(t))
        else:
            self.printh("#define UA_" + self.parser.outname.upper() + " NULL")

        self.printh('''
_UA_END_DECLS

#endif /* %s_GENERATED_H_ */\n''' % self.parser.outname.upper())

    def print_doc(self):
        for ns in self.filtered_types:
            for i, t_name in enumerate(self.filtered_types[ns]):
                t = self.filtered_types[ns][t_name]
                if isinstance(t, BuiltinType):
                    continue
                self.printd(t.name)
                self.printd("^" * len(t.name))
                self.printd(t.description)
                self.printd(".. code-block:: c\n")
                lines = self.print_datatype_typedef(t, True)
                for l in lines.splitlines():
                    self.printd("    " + l)
                self.printd("")

    def print_description_array(self):
        self.printc('''/**********************************
 * Autogenerated -- do not modify *
 **********************************/

#include "''' + self.parser.outname + '''_generated.h"''')

        totalCount = 0
        for ns in self.filtered_types:
            totalCount += len(self.filtered_types[ns])
            for i, t_name in enumerate(self.filtered_types[ns]):
                t = self.filtered_types[ns][t_name]
                self.printc("")
                self.printc("/* " + t.name + " */")
                self.printc(CGenerator.print_members(t, self.namespaceMap))

        if totalCount > 0:
            self.printc(
                "UA_DataType UA_{}[UA_{}_COUNT] = {{".format(self.parser.outname.upper(), self.parser.outname.upper()))

            for ns in self.filtered_types:
                for i, t_name in enumerate(self.filtered_types[ns]):
                    t = self.filtered_types[ns][t_name]
                    self.printc("/* " + t.name + " */")
                    self.printc(self.print_datatype(t, self.namespaceMap) + ",")
            self.printc("};\n")

###########################################
# Execute with the command line arguments #
###########################################

args = parser.parse_args()

outname = args.outfile.split("/")[-1]
inname = ', '.join(list(map(lambda x: x.name.split("/")[-1], args.type_bsd)))

namespaceMap = {"http://opcfoundation.org/UA/": 0}
for m in args.namespace_map:
    [idx, ns] = m.split(':', 1)
    namespaceMap[ns] = int(idx)

parser = CSVBSDTypeParser(args.opaque_map, args.selected_types,
                          args.no_builtin, outname, args.import_bsd,
                          args.type_bsd, args.type_csv, args.type_xml,
                          namespaceMap)
parser.create_types()

generator = CGenerator(parser, inname, args.outfile, args.internal, args.gen_doc, namespaceMap)
generator.write_definitions()
