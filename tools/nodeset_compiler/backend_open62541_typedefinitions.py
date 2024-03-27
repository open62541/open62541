from __future__ import print_function
import re
import itertools
import sys
import copy
from collections import OrderedDict

if sys.version_info[0] >= 3:
    from nodeset_compiler.type_parser import BuiltinType, EnumerationType, OpaqueType, StructType
else:
    from type_parser import BuiltinType, EnumerationType, OpaqueType, StructType

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
        return "UA_NODEIDTYPE_NUMERIC, {{{0}LU}}".format(nodeId)
    if nodeId.startswith("i="):
        return "UA_NODEIDTYPE_NUMERIC, {{{0}LU}}".format(nodeId[2:])
    if nodeId.startswith("s="):
        strId = nodeId[2:]
        return "UA_NODEIDTYPE_STRING, {{ .string = UA_STRING_STATIC(\"{id}\") }}".format(id=strId.replace("\"", "\\\""))

class CGenerator(object):
    def __init__(self, parser, inname, outfile, is_internal_types, gen_doc, namespaceMap):
        self.parser = parser
        self.inname = inname
        self.outfile = outfile
        self.is_internal_types = is_internal_types
        self.gen_doc = gen_doc
        self.filtered_types = None
        self.namespaceMap = namespaceMap
        self.fh = None
        self.ff = None
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
        typeid = "{%s, %s}" % ("0", getNodeidTypeAndId(datatype.nodeId))
        binaryEncodingId = "{%s, %s}" % ("0",
                                         getNodeidTypeAndId(datatype.binaryEncodingId))
        idName = makeCIdentifier(datatype.name)
        pointerfree = "true" if datatype.pointerfree else "false"
        return "{\n" + \
               "    UA_TYPENAME(\"%s\") /* .typeName */\n" % idName + \
               "    " + typeid + ", /* .typeId */\n" + \
               "    " + binaryEncodingId + ", /* .binaryEncodingId */\n" + \
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
        members = "static UA_DataTypeMember %s_members[%s] = {" % (idName, len(datatype.members))
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
            m += "    &UA_%s[UA_%s_%s], /* .memberType */\n" % (
                member.member_type.outname.upper(), member.member_type.outname.upper(),
                makeCIdentifier(type_name.upper()))
            m += "    "
            if not before and not isUnion:
                m += "0,"
            elif isUnion:
                    m += "offsetof(UA_%s, fields.%s)," % (idName, member_name)
            else:
                if member.is_array:
                    m += "offsetof(UA_%s, %sSize)" % (idName, member_name)
                else:
                    m += "offsetof(UA_%s, %s)" % (idName, member_name)
                m += " - offsetof(UA_%s, %s)" % (idName, makeCIdentifier(before.name))
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
        funcs = "UA_INLINABLE( void\nUA_%s_init(UA_%s *p) ,{\n    memset(p, 0, sizeof(UA_%s));\n})\n\n" % (
            idName, idName, idName)
        funcs += "UA_INLINABLE( UA_%s *\nUA_%s_new(void) ,{\n    return (UA_%s*)UA_new(%s);\n})\n\n" % (
            idName, idName, idName, CGenerator.print_datatype_ptr(datatype))
        if datatype.pointerfree == "true":
            funcs += "UA_INLINABLE( UA_StatusCode\nUA_%s_copy(const UA_%s *src, UA_%s *dst) ,{\n    *dst = *src;\n    return UA_STATUSCODE_GOOD;\n})\n\n" % (
                idName, idName, idName)
            funcs += "UA_DEPRECATED UA_INLINABLE( void\nUA_%s_deleteMembers(UA_%s *p) ,{\n    memset(p, 0, sizeof(UA_%s));\n})\n\n" % (
                idName, idName, idName)
            funcs += "UA_INLINABLE( void\nUA_%s_clear(UA_%s *p) ,{\n    memset(p, 0, sizeof(UA_%s));\n})\n\n" % (
                idName, idName, idName)
        else:
            for entry in whitelistFuncAttrWarnUnusedResult:
                if idName == entry:
                    funcs += "UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT "
                    break

            funcs += "UA_INLINABLE( UA_StatusCode\nUA_%s_copy(const UA_%s *src, UA_%s *dst) ,{\n    return UA_copy(src, dst, %s);\n})\n\n" % (
                idName, idName, idName, self.print_datatype_ptr(datatype))
            funcs += "UA_DEPRECATED UA_INLINABLE( void\nUA_%s_deleteMembers(UA_%s *p) ,{\n    UA_clear(p, %s);\n})\n\n" % (
                idName, idName, self.print_datatype_ptr(datatype))
            funcs += "UA_INLINABLE( void\nUA_%s_clear(UA_%s *p) ,{\n    UA_clear(p, %s);\n})\n\n" % (
                idName, idName, self.print_datatype_ptr(datatype))
        funcs += "UA_INLINABLE( void\nUA_%s_delete(UA_%s *p) ,{\n    UA_delete(p, %s);\n})" % (
            idName, idName, self.print_datatype_ptr(datatype))
        funcs += "static UA_INLINE UA_Boolean\nUA_%s_equal(const UA_%s *p1, const UA_%s *p2) {\n    return (UA_order(p1, p2, %s) == UA_ORDER_EQ);\n}\n\n" % (
            idName, idName, idName, self.print_datatype_ptr(datatype))
        return funcs

    def print_datatype_encoding(self, datatype):
        idName = makeCIdentifier(datatype.name)
        enc = "UA_INLINABLE( size_t\nUA_%s_calcSizeBinary(const UA_%s *src) ,{\n    return UA_calcSizeBinary(src, %s);\n})\n"
        enc += "UA_INLINABLE( UA_StatusCode\nUA_%s_encodeBinary(const UA_%s *src, UA_Byte **bufPos, const UA_Byte *bufEnd) ,{\n    return UA_encodeBinary(src, %s, bufPos, &bufEnd, NULL, NULL);\n})\n"
        enc += "UA_INLINABLE( UA_StatusCode\nUA_%s_decodeBinary(const UA_ByteString *src, size_t *offset, UA_%s *dst) ,{\n    return UA_decodeBinary(src, offset, dst, %s, NULL);\n})"
        return enc % tuple(
            list(itertools.chain(*itertools.repeat([idName, idName, self.print_datatype_ptr(datatype)], 3))))

    @staticmethod
    def print_enum_typedef(enum, gen_doc=False):
        if sys.version_info[0] < 3:
            values = enum.elements.iteritems()
        else:
            values = enum.elements.items()

        if enum.isOptionSet == True:
            elements = map(lambda kv: "#define " + makeCIdentifier("UA_" + enum.name.upper() + "_" + kv[0].upper()) + " " + kv[1], values)
            return "typedef " + enum.strDataType + " " + makeCIdentifier("UA_" + enum.name) + ";\n\n" + "\n".join(elements)
        else:
            elements = [makeCIdentifier("UA_" + enum.name.upper() + "_" + kv[0].upper()) + " = " + kv[1] for kv in values]
            if not gen_doc:
                elements.append("__UA_{}_FORCE32BIT = 0x7fffffff".format(makeCIdentifier(enum.name.upper())))
            out = []
            out.append("typedef enum {")
            for i,e in enumerate(elements):
                out.append("    " + e)
                if i < len(elements)-1:
                    out[-1] += ","
            out.append("}} UA_{0};".format(makeCIdentifier(enum.name)))
            if not gen_doc:
                out.append("\nUA_STATIC_ASSERT(sizeof(UA_{0}) == sizeof(UA_Int32), enum_must_be_32bit);".format(makeCIdentifier(enum.name)))
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
            returnstr += "typedef struct UA_%s UA_%s;\n" % (makeCIdentifier(struct.name), makeCIdentifier(struct.name))
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
                returnstr += "    UA_%s *%s;\n" % (
                    makeCIdentifier(type_name), makeCIdentifier(member.name))
                if struct.is_union:
                    returnstr += "        } " + makeCIdentifier(member.name) + ";\n"
            elif struct.is_union:
                returnstr += "        UA_%s %s;\n" % (
                makeCIdentifier(type_name), makeCIdentifier(member.name))
            elif member.is_optional:
                returnstr += "    UA_%s *%s;\n" % (
                    makeCIdentifier(type_name), makeCIdentifier(member.name))
            else:
                returnstr += "    UA_%s %s;\n" % (
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
        self.ff = open(self.outfile + "_generated_handling.h", 'w')
        self.fc = open(self.outfile + "_generated.c", 'w')

        self.filtered_types = self.iter_types(self.parser.types)

        self.print_header()
        self.print_handling()
        self.print_description_array()

        self.fh.close()
        self.ff.close()
        self.fc.close()

        if self.gen_doc:
            self.fd = open(self.outfile + "_generated.rst", 'w')
            self.print_doc()
            self.fd.close()

    def printh(self, string):
        print(string, end='\n', file=self.fh)

    def printf(self, string):
        print(string, end='\n', file=self.ff)

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
            
        self.printh(u'''/**********************************
 * Autogenerated -- do not modify *
 **********************************/

/* Must be before the include guards */
#ifdef UA_ENABLE_AMALGAMATION
# include "open62541.h"
#else
# include <open62541/types.h>
#endif

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
                    self.printh(
                        "#define UA_" + makeCIdentifier(self.parser.outname.upper() + "_" + t.name.upper()) + " " + str(i))
        else:
            self.printh("#define UA_" + self.parser.outname.upper() + " NULL")

        self.printh('''

_UA_END_DECLS

#endif /* %s_GENERATED_H_ */''' % self.parser.outname.upper())

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

    def print_handling(self):
        self.printf(u'''/**********************************
 * Autogenerated -- do not modify *
 **********************************/

#ifndef ''' + self.parser.outname.upper() + '''_GENERATED_HANDLING_H_
#define ''' + self.parser.outname.upper() + '''_GENERATED_HANDLING_H_

#include "''' + self.parser.outname + '''_generated.h"

_UA_BEGIN_DECLS

#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
''')

        for ns in self.filtered_types:
            for i, t_name in enumerate(self.filtered_types[ns]):
                t = self.filtered_types[ns][t_name]
                self.printf("\n/* " + t.name + " */")
                self.printf(self.print_functions(t))

        self.printf('''
#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
# pragma GCC diagnostic pop
#endif

_UA_END_DECLS

#endif /* %s_GENERATED_HANDLING_H_ */''' % self.parser.outname.upper())

    def print_description_array(self):
        self.printc(u'''/**********************************
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
                "UA_DataType UA_%s[UA_%s_COUNT] = {" % (self.parser.outname.upper(), self.parser.outname.upper()))

            for ns in self.filtered_types:
                for i, t_name in enumerate(self.filtered_types[ns]):
                    t = self.filtered_types[ns][t_name]
                    self.printc("/* " + t.name + " */")
                    self.printc(self.print_datatype(t, self.namespaceMap) + ",")
            self.printc("};\n")
