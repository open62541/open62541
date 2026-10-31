#!/usr/bin/env python3

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)

import re
import ntpath
import copy
from collections import OrderedDict
import argparse
import base64
import uuid

from .type_parser import (BuiltinType, CSVBSDTypeParser, EnumerationType,
                          OpaqueType, StructType)


def portable_basename(path):
    """Return the final component for native, POSIX, or Windows paths."""
    return ntpath.basename(path)

###############################
# Parse the Command Line Input#
###############################

def build_argument_parser():
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
                             'Parameter can be used multiple times to define multiple mappings. '
                             'Pinning the index of every namespace that contributes a type makes the generated type array const. '
                             'Purely imported namespaces need not be pinned. An incomplete or inconsistent mapping is an error.')

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

    parser.add_argument('outfile',
                        metavar='<outputFile>',
                        help='output file w/o extension')
    return parser

###################
# Code Generation #
###################

# Namespace 0, always at index 0 in a server
OPC_UA_NAMESPACE = "http://opcfoundation.org/UA/"

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
                       "StatusCode": "UA_BINARY_OVERLAYABLE_INTEGER",
                       "Guid": "(UA_BINARY_OVERLAYABLE_INTEGER && " +
                               "offsetof(UA_Guid, data2) == sizeof(UA_UInt32) && " +
                               "offsetof(UA_Guid, data3) == (sizeof(UA_UInt16) + sizeof(UA_UInt32)) && " +
                               "offsetof(UA_Guid, data4) == (2*sizeof(UA_UInt32)))"}

whitelistFuncAttrWarnUnusedResult = []  # for instances [ "String", "ByteString", "LocalizedText" ]


# Escape C strings:
def makeCLiteral(value):
    return value.replace('\\', r'\\').replace('"', r'\"').replace('\n', r'\n').replace('\r', r'\r')

# Strip invalid characters to create valid C identifiers (variable names etc):
def makeCIdentifier(value):
    keywords = frozenset((
        "auto break case char const continue default do double else enum extern float for goto if inline "
        "int long register restrict return short signed sizeof static struct switch typedef union "
        "unsigned void volatile while _Alignas _Alignof _Atomic _Bool _Complex _Generic _Imaginary "
        "_Noreturn _Static_assert _Thread_local _BitInt _Decimal32 _Decimal64 _Decimal128 typeof "
        "typeof_unqual alignas alignof and and_eq asm bitand bitor bool catch char8_t char16_t char32_t "
        "class compl concept consteval constexpr constinit const_cast co_await co_return co_yield "
        "decltype delete dynamic_cast explicit export false friend mutable namespace new noexcept not "
        "not_eq nullptr operator or or_eq private protected public reinterpret_cast requires "
        "static_assert static_cast template this thread_local throw true try typeid typename using "
        "virtual wchar_t xor xor_eq "
    ).split())
    sanitized = re.sub(r'[^\w]', '', value)
    if not sanitized or sanitized[0].isdigit() or sanitized in keywords:
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
        return "UA_NODEIDTYPE_STRING, {{ .string = UA_STRING_STATIC(\"{id}\") }}".format(id=makeCLiteral(strId))

    if nodeId.startswith("g="):
        guid = uuid.UUID(nodeId[2:])
        fields = guid.fields
        tail = ", ".join("0x%02x" % byte for byte in guid.bytes[8:])
        return ("UA_NODEIDTYPE_GUID, { .guid = {0x%08x, 0x%04x, 0x%04x, {%s}} }" %
                (fields[0], fields[1], fields[2], tail))
    if nodeId.startswith("b="):
        data = base64.b64decode(nodeId[2:], validate=True)
        literal = "".join("\\%03o" % byte for byte in data)
        return ('UA_NODEIDTYPE_BYTESTRING, { .byteString = {%d, (UA_Byte*)"%s"} }' %
                (len(data), literal))
    raise RuntimeError("Unsupported NodeId: " + nodeId)

def splitNodeidNs(nodeId):
    """Split an optional 'ns=X;' prefix from a nodeId string.
    Returns (namespaceindex_str, bare_nodeId)."""
    if nodeId is not None and not isinstance(nodeId, str):
        nodeId = str(nodeId)
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
    def __init__(self, parser, inname, outfile, is_internal_types, gen_doc, namespaceMap, export_macro=""):
        self.parser = parser
        self.inname = inname
        self.outfile = outfile
        self.is_internal_types = is_internal_types
        self.gen_doc = gen_doc
        self.filtered_types = None
        self.namespaceMap = namespaceMap
        self.export_macro = export_macro if export_macro else "UA_EXPORT"
        self.fh = None
        self.fc = None
        self.fd = None
        self.fe = None

    def _contributing_namespaces(self):
        """The namespaces that actually contribute types to the generated
        array. iter_types drops types that come from imported bsd files but
        keeps the (then empty) namespace entry, so filtered_types can list
        namespaces that own no type in this array. Those do not constrain the
        baked namespace indices."""
        namespaces = [ns for ns, types in self.filtered_types.items() if types]
        uris = getattr(self.parser, "namespaceUris", None)
        if uris is not None:
            for types in self.filtered_types.values():
                for datatype in types.values():
                    for nodeid in (datatype.binaryEncodingId, datatype.xmlEncodingId):
                        ns, bare = splitNodeidNs(nodeid)
                        if not bare or (ns == "0" and bare == "i=0"):
                            continue
                        uri = uris[int(ns)]
                        if uri not in namespaces:
                            namespaces.append(uri)
        return namespaces

    def _validate_namespace_map(self):
        """A fixed map must cover every namespace used by this array.

        Extra entries allow one application-wide plan to be shared by all
        generated modules. An absent map makes the array copy-only.
        """
        if self.namespaceMap == {OPC_UA_NAMESPACE: 0}:
            return  # Nothing pinned, only the implicit namespace 0 entry

        byIndex = {}
        for ns in sorted(self.namespaceMap):
            idx = self.namespaceMap[ns]
            if not 0 <= idx <= 65535:
                raise RuntimeError(
                    "--namespaceMap index {} for {} is outside the UInt16 "
                    "range (0..65535).".format(idx, ns))
            if idx in byIndex:
                raise RuntimeError(
                    "--namespaceMap pins index {} to more than one namespace:\n"
                    "  {}\n  {}".format(idx, byIndex[idx], ns))
            byIndex[idx] = ns

        missing = [ns for ns in self._contributing_namespaces()
                   if ns not in self.namespaceMap]
        if missing:
            raise RuntimeError(
                "--namespaceMap is incomplete. Pinning is all-or-nothing: as "
                "long as one namespace is unpinned the type array cannot be "
                "registered directly. Either pin all "
                "namespaces that contribute types or drop --namespaceMap.\n"
                "Not pinned:\n" +
                "".join("  {}\n".format(ns) for ns in sorted(missing)))

    def _can_be_const(self):
        """Whether all identities are pinned for direct static registration."""
        return all(ns in self.namespaceMap for ns in self._contributing_namespaces())

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

    def _baked_nodeid(self, datatype, nodeIdStr):
        ns, bare = splitNodeidNs(nodeIdStr)
        if not bare or (ns == "0" and bare == "i=0"):
            return "0", bare
        uris = getattr(self.parser, "namespaceUris", None)
        uri = uris[int(ns)] if uris is not None else datatype.namespaceUri
        if uris is None and int(ns):
            uri = next((u for u, i in self.namespaceMap.items() if i == int(ns)), uri)
        return str(self.bakedNamespaceMap[uri]), bare

    def print_datatype(self, datatype):
        nodeid = self._baked_nodeid
        nsIdx, bareNodeId = nodeid(datatype, datatype.nodeId)
        typeid = "{{{}, {}}}".format(nsIdx, getNodeidTypeAndId(bareNodeId))
        binNs, bareBinId = nodeid(datatype, datatype.binaryEncodingId)
        binaryEncodingId = "{{{}, {}}}".format(binNs, getNodeidTypeAndId(bareBinId))
        xmlNs, bareXmlId = nodeid(datatype, datatype.xmlEncodingId)
        xmlEncodingId = "{{{}, {}}}".format(xmlNs, getNodeidTypeAndId(bareXmlId))

        idName = makeCIdentifier(datatype.name)
        pointerfree = "true" if datatype.pointerfree else "false"
        # TODO: OptionSet is omitted because the type description is not generated as UA_DATATYPEKIND_ENUM
        isEnum = isinstance(datatype, EnumerationType)  and not datatype.isOptionSet
        return "{\n" + \
               "    UA_TYPENAME(\"%s\") /* .typeName */\n" % makeCLiteral(getattr(datatype, "wire_name", getattr(datatype, "original_name", datatype.name))) + \
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

    @staticmethod
    def print_members(datatype):
        idName = makeCIdentifier(datatype.name)
        # TODO: OptionSet is omitted because the type description is not generated as UA_DATATYPEKIND_ENUM
        isEnum = isinstance(datatype, EnumerationType) and not datatype.isOptionSet
        if (not isEnum and len(datatype.members) == 0) or (isEnum and len(datatype.elements) == 0):
            return "#define %s_members NULL" % (idName)
        isUnion = isinstance(datatype, StructType) and datatype.is_union
        members = "static const UA_DataTypeMember {}_members[{}] = {{".format(idName, len(datatype.elements) if isEnum else len(datatype.members))
        before = None
        size = len(datatype.elements) if isEnum else len(datatype.members)
        if isEnum:
            # Print all enumerators as UA_DataTypeMember
            for i, (name, value) in enumerate(datatype.elements.items()):
                m = "\n{\n"
                m += "    UA_TYPENAME(\"%s\") /* .memberName */\n" % name
                m += "    (const UA_DataType *)(uintptr_t)({}), /* .memberType */\n".format(value)
                m += "    0, /* .padding */\n"
                m += "    false, /* .isArray */\n"
                m += "    false /* .isOptional */\n}"
                # Don't add a trailing comma for the last entry
                if i != size - 1:
                    m += ","
                members += m
        else:
            # Print all structure fields as UA_DataTypeMember
            for i, member in enumerate(datatype.members):

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
                m = "\n{\n"
                m += "    UA_TYPENAME(\"%s\") /* .memberName */\n" % makeCLiteral(getattr(member, "original_name", member_name_capital))
                m += "    &UA_{}[UA_{}_{}], /* .memberType */\n".format(
                    member.member_type.outname.upper(), member.member_type.outname.upper(),
                    re.sub(r"[^\w]", "", type_name.upper()))
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
                # Don't add a trailing comma for the last entry
                if i != size - 1:
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
        values = list(enum.elements.items())
        names = []
        used = set()
        for index, (label, _) in enumerate(values):
            name = makeCIdentifier("UA_" + enum.name.upper() + "_" + label.upper())
            candidate = name
            suffix = index
            while candidate in used:
                candidate = name + "_" + str(suffix)
                suffix += 1
            used.add(candidate)
            names.append(candidate)
        if enum.isOptionSet:
            elements = ["#define " + name + " " + value
                        for name, (_, value) in zip(names, values)]
            return "typedef " + enum.strDataType + " " + makeCIdentifier("UA_" + enum.name) + ";\n\n" + "\n".join(elements)
        else:
            elements = [name + " = " + value for name, (_, value) in zip(names, values)]
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
            obj = type('MyEnumOptionSet', (object,), {'isOptionSet': False, 'elements': OrderedDict(), 'name': makeCIdentifier(struct.name)+"Switch"})
            obj.elements['None'] = str(0)
            count = 1
            for member in struct.members:
                obj.elements[member.name] = str(count)
                count += 1
            returnstr += CGenerator.print_enum_typedef(obj)
            returnstr += "\n\n"
        if len(struct.members) == 0:
            raise Exception("Structs with no members are filtered out. Why not here?")
        if getattr(struct, "forward_declared", False):
            returnstr += "struct UA_%s {\n" % makeCIdentifier(struct.name)
        elif struct.is_recursive:
            returnstr += "typedef struct UA_{} UA_{};\n".format(makeCIdentifier(struct.name), makeCIdentifier(struct.name))
            returnstr += "struct UA_%s {\n" % makeCIdentifier(struct.name)
        else:
            returnstr += "typedef struct {\n"
        if struct.is_union:
            returnstr += "    UA_%sSwitch switchField;\n" % makeCIdentifier(struct.name)
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
        if struct.is_recursive or getattr(struct, "forward_declared", False):
            return returnstr + "};"
        return returnstr + "} UA_%s;" % makeCIdentifier(struct.name)

    @staticmethod
    def print_datatype_typedef(datatype, gen_doc=False):
        if isinstance(datatype, EnumerationType):
            return CGenerator.print_enum_typedef(datatype, gen_doc)
        if isinstance(datatype, OpaqueType):
            return "typedef UA_" + datatype.base_type + " UA_%s;" % makeCIdentifier(datatype.name)
        if isinstance(datatype, StructType):
            return CGenerator.print_struct_typedef(datatype)
        raise RuntimeError("Type does not have an associated typedef")

    def write_definitions(self):
        self.fh = open(self.outfile + "_generated.h", 'w')
        self.fc = open(self.outfile + "_generated.c", 'w')

        self.filtered_types = self.iter_types(self.parser.types)
        self._validate_namespace_map()
        self.bakedNamespaceMap = dict(self.namespaceMap)
        if not self._can_be_const():
            self.bakedNamespaceMap = {OPC_UA_NAMESPACE: 0}
            for uri in self._contributing_namespaces():
                if uri not in self.bakedNamespaceMap:
                    self.bakedNamespaceMap[uri] = len(self.bakedNamespaceMap)

        self.print_header()
        self.print_internal_header()
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
            outUpper = self.parser.outname.upper()
            const_q = "const "
            self.printh("#define UA_" + outUpper + "_IS_CONST 1")
            if self._can_be_const():
                self.printh("""
/* All namespace indices in the type array are fixed at generation time
 * (--namespaceMap). The array is const and can reside in read-only memory.
 * The server must assign exactly the baked indices to these namespace URIs
 * at runtime. The nodeset initializer verifies the URI/index pairs below. */""")
                # Preserve the URI/index association for nodeset initialization.
                # Array order and the nodeset output filename do not identify
                # which namespace a type belongs to.
                entries = [
                    '{{{}, UA_STRING_STATIC("{}")}}'.format(
                        self.namespaceMap[ns], makeCLiteral(ns))
                    for ns in self._contributing_namespaces()]
                self.printh("#define UA_" + outUpper + "_NAMESPACE_MAP {" +
                            ", ".join(entries) + "}")
            self.printh(
                "extern " + self.export_macro + " " + const_q + "UA_DataType UA_" + outUpper + "[UA_" + outUpper + "_COUNT];")

            # NodeSet definitions may contain mutually recursive indirect
            # members. Declare their names before printing complete layouts.
            if hasattr(self.parser, "namespaceUris"):
                for ns_types in self.filtered_types.values():
                    for t in ns_types.values():
                        if (getattr(t, "needs_forward_declaration", False) and
                                t.name not in self.cross_ns_duplicate_types):
                            t.forward_declared = True
                            name = makeCIdentifier(t.name)
                            self.printh("typedef struct UA_%s UA_%s;" % (name, name))

            typeIndex = 0
            for ns in self.filtered_types:
                for t_name in self.filtered_types[ns]:
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
                    self.printh("#define UA_" + makeCIdentifier(self.parser.outname.upper() + "_" + t.name.upper()) + " " + str(typeIndex))
                    typeIndex += 1
                    self.printh("")
                    if not is_cross_ns_dup:
                        self.printh(self.print_functions(t))
        else:
            self.printh("#define UA_" + self.parser.outname.upper() + " NULL")

        self.printh('''
_UA_END_DECLS

#endif /* %s_GENERATED_H_ */\n''' % self.parser.outname.upper())

    def conversion_order(self):
        """Order inline dependencies without changing exported array indexes."""
        types = [t for group in self.filtered_types.values() for t in group.values()]
        indexes = {self.get_type_index(t): i for i, t in enumerate(types)}
        order, active, done = [], set(), set()

        def visit(i):
            if i in done:
                return
            if i in active:
                raise RuntimeError("DataTypes involving %s have a cycle of inline members" % types[i].name)
            active.add(i)
            if isinstance(types[i], StructType):
                for member in types[i].members:
                    dependency = indexes.get(self.get_type_index(member.member_type))
                    if dependency is not None and not member.is_array and not member.is_optional:
                        visit(dependency)
            active.remove(i)
            done.add(i)
            order.append(i)

        for i in range(len(types)):
            visit(i)
        return order

    def print_internal_header(self):
        """Keep registration metadata out of the generated application API."""
        outUpper = self.parser.outname.upper()
        with open(self.outfile + "_generated_internal.h", "w", encoding="utf-8") as header:
            print("/* Generated implementation support. Do not include from applications. */",
                  file=header)
            print("#ifndef " + outUpper + "_GENERATED_INTERNAL_H_", file=header)
            print("#define " + outUpper + "_GENERATED_INTERNAL_H_", file=header)
            print('#include "' + self.parser.outname + '_generated.h"', file=header)
            if self.parser.outname not in ("types", "transport") and any(self.filtered_types.values()):
                size = max(self.bakedNamespaceMap.values()) + 1
                prefix = "UA_" + outUpper
                print("_UA_BEGIN_DECLS", file=header)
                print("extern " + self.export_macro + " const UA_String " + prefix +
                      "_URIS[%d];" % size, file=header)
                print("extern " + self.export_macro + " const size_t " + prefix +
                      "_ORDER[" + prefix + "_COUNT];", file=header)
                print("_UA_END_DECLS", file=header)
                print("#define " + prefix + "_SOURCE {" + prefix + ", " + prefix +
                      "_COUNT, " + prefix + "_URIS, %d, %s, " %
                      (size, "true" if self._can_be_const() else "false") +
                      prefix + "_ORDER}", file=header)
            print("#endif /* " + outUpper + "_GENERATED_INTERNAL_H_ */", file=header)

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
        if self.parser.outname not in ("types", "transport") and any(self.filtered_types.values()):
            self.printc('#include "' + self.parser.outname + '_generated_internal.h"')

        totalCount = 0
        for ns in self.filtered_types:
            totalCount += len(self.filtered_types[ns])
            for _, t_name in enumerate(self.filtered_types[ns]):
                t = self.filtered_types[ns][t_name]
                self.printc("")
                self.printc("/* " + t.name + " */")
                self.printc(CGenerator.print_members(t))

        if totalCount > 0:
            outUpper = self.parser.outname.upper()
            const_q = "const "
            self.printc(
                "{}UA_DataType UA_{}[UA_{}_COUNT] = {{".format(const_q, outUpper, outUpper))

            for ns in self.filtered_types:
                for _, t_name in enumerate(self.filtered_types[ns]):
                    t = self.filtered_types[ns][t_name]
                    self.printc("/* " + t.name + " */")
                    self.printc(self.print_datatype(t) + ",")
            self.printc("};\n")
            if self.parser.outname not in ("types", "transport"):
                self.printc("const UA_String UA_" + outUpper + "_URIS[] = {")
                reverse = {i: uri for uri, i in self.bakedNamespaceMap.items()}
                for index in range(max(reverse) + 1):
                    uri = reverse.get(index)
                    self.printc('    UA_STRING_STATIC("%s"),' % makeCLiteral(uri) if uri else
                                '    UA_STRING_STATIC(""),')
                self.printc("};")
                self.printc("const size_t UA_" + outUpper + "_ORDER[] = {" +
                            ", ".join(map(str, self.conversion_order())) + "};")


###########################################
# Execute with the command line arguments #
###########################################

def main(argv=None):
    parser = build_argument_parser()
    args = parser.parse_args(argv)

    outname = portable_basename(args.outfile)
    inname = ', '.join(portable_basename(x.name) for x in args.type_bsd)

    namespaceMap = {OPC_UA_NAMESPACE: 0}
    for m in args.namespace_map:
        [idx, ns] = m.split(':', 1)
        idx = int(idx)
        if ns in namespaceMap and namespaceMap[ns] != idx:
            parser.error("--namespaceMap pins {} to conflicting indices {} and {}."
                         .format(ns, namespaceMap[ns], idx))
        namespaceMap[ns] = idx

    typeParser = CSVBSDTypeParser(args.opaque_map, args.selected_types,
                                  args.no_builtin, outname, args.import_bsd,
                                  args.type_bsd, args.type_csv, args.type_xml,
                                  namespaceMap)
    typeParser.create_types()

    generator = CGenerator(typeParser, inname, args.outfile, args.internal,
                           args.gen_doc, namespaceMap, args.export_macro)
    generator.write_definitions()
