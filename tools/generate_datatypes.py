#!/usr/bin/env python3

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)

import re
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

parser.add_argument('--export-macro',
                    metavar="<exportMacro>",
                    type=str,
                    dest="export_macro",
                    default="",
                    help='macro to use in front of extern declarations (default: UA_EXPORT)')

parser.add_argument('--members-extram-attr',
                    metavar="<attrMacro>",
                    type=str,
                    dest="members_extram_attr",
                    default="",
                    help='if set, declare the per-type *_members arrays (UA_DataTypeMember[]) '
                         'zero-initialized with this attribute macro and fill them in via a '
                         'generated __attribute__((constructor)) function instead of a '
                         'compile-time initializer, so they can be attribute-placed in external '
                         'RAM on targets where only zero-initialized (.bss) data can be (e.g. '
                         'pass EXT_RAM_BSS_ATTR on ESP-IDF with '
                         'CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY). Default: unset, arrays '
                         'stay compile-time-initialized as before.')

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

def splitNodeidNs(nodeId):
    """Split an optional 'ns=X;' prefix from a nodeId string.
    Returns (namespaceindex_str, bare_nodeId)."""
    if nodeId and nodeId.startswith("ns="):
        parts = nodeId.split(";", 1)
        return parts[0][3:], parts[1] if len(parts) > 1 else ""
    return "0", nodeId

def _types_definition_equal(t1, t2):
    """Compare two Type objects by structural definition (ignoring nodeId/outname).
    Used to detect cross-namespace same-name types that are ABI-compatible vs
    those that would produce a silent memory-layout mismatch."""
    if type(t1) is not type(t2):
        return False
    if isinstance(t1, EnumerationType):
        return (t1.elements == t2.elements and
                t1.isOptionSet == t2.isOptionSet and
                t1.lengthInBits == t2.lengthInBits)
    if isinstance(t1, OpaqueType):
        return t1.base_type == t2.base_type
    if isinstance(t1, StructType):
        if len(t1.members) != len(t2.members):
            return False
        for m1, m2 in zip(t1.members, t2.members):
            if (m1.name != m2.name or
                    m1.member_type.name != m2.member_type.name or
                    m1.is_array != m2.is_array or
                    m1.is_optional != m2.is_optional):
                return False
        return True
    return False

class CGenerator:
    def __init__(self, parser, inname, outfile, is_internal_types, gen_doc, namespaceMap, export_macro="", members_extram_attr=""):
        self.parser = parser
        self.inname = inname
        self.outfile = outfile
        self.is_internal_types = is_internal_types
        self.gen_doc = gen_doc
        self.filtered_types = None
        self.namespaceMap = namespaceMap
        self.export_macro = export_macro if export_macro else "UA_EXPORT"
        # If set, the *_members arrays (UA_DataTypeMember[]) are declared
        # zero-initialized with this attribute macro and filled in by a
        # generated __attribute__((constructor)) function instead of a
        # compile-time initializer -- see print_members(). Intended for
        # e.g. "EXT_RAM_BSS_ATTR" on ESP-IDF, to place them in external
        # PSRAM (only .bss/zero-initialized data can be attribute-placed
        # in PSRAM there) instead of internal RAM. Empty/default: no
        # change from the plain compile-time-initialized array.
        self.members_extram_attr = members_extram_attr
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

    def print_datatype(self, datatype):
        nsIdx, bareNodeId = splitNodeidNs(datatype.nodeId)
        typeid = "{{{}, {}}}".format(nsIdx, getNodeidTypeAndId(bareNodeId))
        binNs, bareBinId = splitNodeidNs(datatype.binaryEncodingId)
        binaryEncodingId = "{{{}, {}}}".format(binNs, getNodeidTypeAndId(bareBinId))
        xmlNs, bareXmlId = splitNodeidNs(datatype.xmlEncodingId)
        xmlEncodingId = "{{{}, {}}}".format(xmlNs, getNodeidTypeAndId(bareXmlId))
        idName = makeCIdentifier(datatype.name)
        pointerfree = "true" if datatype.pointerfree else "false"
        # TODO: OptionSet is omitted because the type description is not generated as UA_DATATYPEKIND_ENUM
        isEnum = isinstance(datatype, EnumerationType)  and not datatype.isOptionSet
        return "{\n" + \
               "    UA_TYPENAME(\"%s\") /* .typeName */\n" % idName + \
               "    " + typeid + ", /* .typeId */\n" + \
               "    " + binaryEncodingId + ", /* .binaryEncodingId */\n" + \
               "    " + xmlEncodingId + ", /* .xmlEncodingId */\n" + \
               "    sizeof(UA_" + idName + "), /* .memSize */\n" + \
               "    " + self.get_type_kind(datatype) + ", /* .typeKind */\n" + \
               "    " + pointerfree + ", /* .pointerFree */\n" + \
               "    " + self.get_type_overlayable(datatype) + ", /* .overlayable */\n" + \
               "    " + str(len(datatype.elements) if isEnum else len(datatype.members)) + ", /* .membersSize */\n" + \
               "    %s_members" % idName + "  /* .members */\n" + \
               "}"

    def print_members(self, datatype):
        idName = makeCIdentifier(datatype.name)
        # TODO: OptionSet is omitted because the type description is not generated as UA_DATATYPEKIND_ENUM
        isEnum = isinstance(datatype, EnumerationType) and not datatype.isOptionSet
        if (not isEnum and len(datatype.members) == 0) or (isEnum and len(datatype.elements) == 0):
            return "#define %s_members NULL" % (idName), None
        isUnion = isinstance(datatype, StructType) and datatype.is_union
        size = len(datatype.elements) if isEnum else len(datatype.members)

        # Build the per-element initializer bodies ("{ ... }", no trailing
        # comma). Used verbatim both for the plain aggregate initializer
        # (default) and, one element at a time, for the runtime init
        # function (self.members_extram_attr set -- see below).
        bodies = []
        before = None
        if isEnum:
            # Print all enumerators as UA_DataTypeMember
            for name, value in datatype.elements.items():
                m = "{\n"
                m += "    UA_TYPENAME(\"%s\") /* .memberName */\n" % name
                m += "    (const UA_DataType *)(uintptr_t)({}), /* .memberType */\n".format(value)
                m += "    0, /* .padding */\n"
                m += "    false, /* .isArray */\n"
                m += "    false /* .isOptional */\n}"
                bodies.append(m)
        else:
            # Print all structure fields as UA_DataTypeMember
            for member in datatype.members:

                # Build the type name for the current field
                if not member.member_type.members and isinstance(member.member_type, StructType):
                    type_name = "ExtensionObject"
                else:
                    type_name = member.member_type.name

                # Build the type name for the previous field (used to calculate padding)
                if before:
                    if not before.member_type.members and isinstance(before.member_type, StructType):
                        type_name_before = "ExtensionObject"
                    else:
                        type_name_before = before.member_type.name

                # Build a valid identifier as member name with capital first letter
                member_name = makeCIdentifier(member.name)
                member_name_capital = member_name
                if len(member_name) > 0:
                    member_name_capital = member_name[0].upper() + member_name[1:]
                m = "{\n"
                m += "    UA_TYPENAME(\"%s\") /* .memberName */\n" % member_name_capital
                m += "    &UA_{}[UA_{}_{}], /* .memberType */\n".format(
                    member.member_type.outname.upper(), member.member_type.outname.upper(),
                    makeCIdentifier(type_name.upper()))
                m += "    "
                # Print code to calculate type specific padding for the member
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
                bodies.append(m)
                before = member

        if not self.members_extram_attr:
            # Default: a single plain aggregate initializer, exactly as before.
            members = "static UA_DataTypeMember {}_members[{}] = {{".format(idName, size)
            members += ",".join("\n" + b for b in bodies)
            return members + "};", None

        # --members-extram-attr set: declare as a zero-initialized array
        # tagged with the given attribute (e.g. EXT_RAM_BSS_ATTR on
        # ESP-IDF, to place it in external PSRAM instead of internal RAM)
        # and fill it in via a generated one-time init function instead of
        # a compile-time initializer. Zero-initialized (.bss) placement is
        # the only form ESP-IDF's EXT_RAM_BSS_ATTR supports; a regular
        # non-zero initializer would stay internal (.data) regardless of
        # the attribute. The values themselves are unchanged (still
        # link-time constants -- pointers into the const UA_TYPES-style
        # arrays and offsetof()/sizeof() expressions), just written via
        # assignment instead of aggregate-initialized.
        decl = "static {} UA_DataTypeMember {}_members[{}];".format(
            self.members_extram_attr, idName, size)
        init_fn_name = "{}_members_init".format(idName)
        init_fn = "static void {}(void) {{\n".format(init_fn_name)
        for i, b in enumerate(bodies):
            init_fn += "    {}_members[{}] = (UA_DataTypeMember){};\n".format(idName, i, b)
        init_fn += "}"
        return decl + "\n" + init_fn, init_fn_name

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
        if enum.isOptionSet:
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

        # Remove types that are from other (imported) bsd files.
        # Track type names from imported namespaces so we can detect
        # cross-namespace name collisions (same C type name, different
        # OPC UA namespace).  Those types must still appear in the type
        # array (they have their own NodeId), but we must not re-emit
        # the C typedef / inline helpers because the C symbol already
        # exists from the included dependency header.
        # IMPORTANT: the definitions must be identical; differing definitions
        # would cause a silent ABI mismatch since the same C struct is used for
        # both type array entries.
        self.cross_ns_duplicate_types = set()
        # Build name→Type map from existing (imported) types for comparison.
        # When the same name appears in multiple imported namespaces prefer the
        # non-builtin definition.
        existing_type_map = {}
        for ns in self.parser.existing_types:
            for t in self.parser.existing_types[ns]:
                et = self.parser.existing_types[ns][t]
                if t not in existing_type_map or isinstance(existing_type_map[t], BuiltinType):
                    existing_type_map[t] = et
                if ns in l and t in l[ns]:
                    del l[ns][t]
        for ns in list(l.keys()):
            for t in list(l.get(ns, {}).keys()):
                if t in existing_type_map:
                    clash = existing_type_map[t]
                    if not isinstance(clash, BuiltinType) and \
                            not _types_definition_equal(clash, l[ns][t]):
                        raise RuntimeError(
                            f"Type '{t}' in namespace '{ns}' has the same name "
                            f"as an imported type but a different definition. "
                            f"Cross-namespace duplicate type names are only "
                            f"allowed when definitions are identical.\n"
                            f"  Imported from: {clash.namespaceUri}\n"
                            f"  Current:       {ns}"
                        )
                    self.cross_ns_duplicate_types.add(t)

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
                "extern " + self.export_macro + " UA_DataType UA_" + self.parser.outname.upper() + "[UA_" + self.parser.outname.upper() + "_COUNT];")

            for ns in self.filtered_types:
                for i, t_name in enumerate(self.filtered_types[ns]):
                    t = self.filtered_types[ns][t_name]
                    is_cross_ns_dup = t_name in self.cross_ns_duplicate_types
                    if t.description == "":
                        self.printh("\n/* " + t.name + " */")
                    else:
                        self.printh("\n/* " + t.name + ": " + t.description + " */")
                    # For cross-namespace duplicates the C typedef and inline
                    # helpers already exist from the imported dependency header.
                    # We only emit the index constant here; the type array
                    # entry is emitted in the .c file as usual.
                    if not is_cross_ns_dup:
                        if not isinstance(t, BuiltinType):
                            self.printh(self.print_datatype_typedef(t) + "\n")
                    self.printh("#define UA_" + makeCIdentifier(self.parser.outname.upper() + "_" + t.name.upper()) + " " + str(i))
                    self.printh("")
                    if not is_cross_ns_dup:
                        self.printh(self.print_functions(t))
        else:
            self.printh("#define UA_" + self.parser.outname.upper() + " NULL")

        self.printh('''
_UA_END_DECLS

#endif /* %s_GENERATED_H_ */\n''' % self.parser.outname.upper())

    def print_doc(self):
        for ns in self.filtered_types:
            for _, t_name in enumerate(self.filtered_types[ns]):
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

        if self.members_extram_attr:
            # Portable fallback so this file compiles standalone even where
            # the attribute macro isn't otherwise defined (e.g. non-ESP-IDF
            # builds): downstream builds that actually want the placement
            # define it (via their own config header included before this
            # file, or a compiler -D flag) to something real, such as
            # EXT_RAM_BSS_ATTR from ESP-IDF's esp_attr.h.
            self.printc('''
#ifndef ''' + self.members_extram_attr + '''
#define ''' + self.members_extram_attr + '''
#endif''')

        totalCount = 0
        memberInitFns = []
        for ns in self.filtered_types:
            totalCount += len(self.filtered_types[ns])
            for _, t_name in enumerate(self.filtered_types[ns]):
                t = self.filtered_types[ns][t_name]
                self.printc("")
                self.printc("/* " + t.name + " */")
                code, initFnName = self.print_members(t)
                self.printc(code)
                if initFnName:
                    memberInitFns.append(initFnName)

        if totalCount > 0:
            self.printc(
                "UA_DataType UA_{}[UA_{}_COUNT] = {{".format(self.parser.outname.upper(), self.parser.outname.upper()))

            for ns in self.filtered_types:
                for _, t_name in enumerate(self.filtered_types[ns]):
                    t = self.filtered_types[ns][t_name]
                    self.printc("/* " + t.name + " */")
                    self.printc(self.print_datatype(t) + ",")
            self.printc("};\n")

        if memberInitFns:
            # Members arrays above were declared zero-initialized (see
            # print_members()); populate them once, automatically, before
            # anything in the program can run -- no explicit call required
            # by servers, clients, or standalone type encode/decode users.
            # Order between these doesn't matter: each function only
            # writes to its own array, using values (pointers into the
            # const UA_TYPES-style arrays, offsetof()/sizeof() constants)
            # that don't depend on any other member array being populated
            # yet.
            self.printc("__attribute__((constructor))")
            self.printc("static void UA_{}_members_init(void) {{".format(self.parser.outname.upper()))
            for fn in memberInitFns:
                self.printc("    {}();".format(fn))
            self.printc("}\n")

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

generator = CGenerator(parser, inname, args.outfile, args.internal, args.gen_doc, namespaceMap, args.export_macro, args.members_extram_attr)
generator.write_definitions()
