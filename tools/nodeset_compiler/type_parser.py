import abc
import csv
import json
import xml.etree.ElementTree as etree
import re
from collections import OrderedDict
import sys

if sys.version_info[0] >= 3:
    from nodeset_compiler.opaque_type_mapping import get_base_type_for_opaque as get_base_type_for_opaque_ns0
else:
    from opaque_type_mapping import get_base_type_for_opaque as get_base_type_for_opaque_ns0
    # import opaque_type_mapping

builtin_types = ["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                 "Int64", "UInt64", "Float", "Double", "String", "DateTime", "Guid",
                 "ByteString", "XmlElement", "NodeId", "ExpandedNodeId", "StatusCode",
                 "QualifiedName", "LocalizedText", "ExtensionObject", "DataValue",
                 "Variant", "DiagnosticInfo"]

excluded_types = ["NodeIdType", "InstanceNode", "TypeNode", "Node", "ObjectNode",
                  "ObjectTypeNode", "VariableNode", "VariableTypeNode", "ReferenceTypeNode",
                  "MethodNode", "ViewNode", "DataTypeNode",
                  "NumericRange", "NumericRangeDimensions",
                  "UA_ServerDiagnosticsSummaryDataType", "UA_SamplingIntervalDiagnosticsDataType",
                  "UA_SessionSecurityDiagnosticsDataType", "UA_SubscriptionDiagnosticsDataType",
                  "UA_SessionDiagnosticsDataType"]

builtin_overlayable = ["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32", "Int64", "UInt64", "Float",
                       "Double", "DateTime", "StatusCode", "Guid"]

# Type aliases
type_aliases = {"CharArray": "String"}

user_opaque_type_mapping = {}  # contains user defined opaque type mapping


def get_base_type_for_opaque(name):
    if name in user_opaque_type_mapping:
        return user_opaque_type_mapping[name]
    else:
        return get_base_type_for_opaque_ns0(name)


def get_type_name(xml_type_name):
    type_name = xml_type_name[xml_type_name.find(":") + 1:]
    return type_aliases.get(type_name, type_name)


class Type(object):
    def __init__(self, outname, xml, namespace):
        self.name = None
        if xml is not None:
            self.name = xml.get("Name")
        self.outname = outname
        self.namespace = namespace
        self.pointerfree = False
        self.members = []
        self.description = ""
        self.ns0 = (namespace == 0)
        if xml is not None:
            for child in xml:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                    self.description = child.text
                    break


class BuiltinType(Type):
    def __init__(self, name):
        Type.__init__(self, "types", None, 0)
        self.name = name
        if self.name in builtin_overlayable:
            self.pointerfree = True


class EnumerationType(Type):
    def __init__(self, outname, xml, namespace):
        Type.__init__(self, outname, xml, namespace)
        self.pointerfree = True
        self.elements = OrderedDict()
        self.isOptionSet = True if xml.get("IsOptionSet", "false") == "true" else False
        self.lengthInBits = 0
        try:
            self.lengthInBits = int(xml.get("LengthInBits", "32"))
        except ValueError as ex:
            raise Exception("Error at EnumerationType '" + self.name + "': 'LengthInBits' XML attribute '" +
                xml.get("LengthInBits") + "' is not convertible to integer. " +
                "Exception: {0}".format(ex));

        # default values for enumerations (encoded as int32):
        self.strDataType = "UA_Int32"
        self.strTypeKind = "UA_DATATYPEKIND_ENUM"
        self.strTypeIndex = "UA_TYPES_INT32"

        # special handling for OptionSet datatype (bitmask)
        if self.isOptionSet == True:
            if self.lengthInBits <= 8:
                self.strDataType = "UA_Byte"
                self.strTypeKind = "UA_DATATYPEKIND_BYTE"
                self.strTypeIndex = "UA_TYPES_BYTE"
            elif self.lengthInBits <= 16:
                self.strDataType = "UA_UInt16"
                self.strTypeKind = "UA_DATATYPEKIND_UINT16"
                self.strTypeIndex = "UA_TYPES_UINT16"
            elif self.lengthInBits <= 32:
                self.strDataType = "UA_UInt32"
                self.strTypeKind = "UA_DATATYPEKIND_UINT32"
                self.strTypeIndex = "UA_TYPES_UINT32"
            elif self.lengthInBits <= 64:
                self.strDataType = "UA_UInt64"
                self.strTypeKind = "UA_DATATYPEKIND_UINT64"
                self.strTypeIndex = "UA_TYPES_UINT64"
            else:
                raise Exception("Error at EnumerationType() CTOR '" + self.name + "': 'LengthInBits' value '" +
                    self.lengthInBits + "' is not supported");

        for child in xml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedValue":
                self.elements[child.get("Name")] = child.get("Value")


class OpaqueType(Type):
    def __init__(self, outname, xml, namespace, base_type):
        Type.__init__(self, outname, xml, namespace)
        self.base_type = base_type


class StructMember(object):
    def __init__(self, name, member_type, is_array, is_optional):
        self.name = name
        self.member_type = member_type
        self.is_array = is_array
        self.is_optional = is_optional


class StructType(Type):
    def __init__(self, outname, xml, namespace, types):
        Type.__init__(self, outname, xml, namespace)
        length_fields = []
        optional_fields = []

        bt = xml.get("BaseType")
        self.is_union = True if bt and get_type_name(bt) == "Union" else False
        for child in xml:
            length_field = child.get("LengthField")
            if length_field:
                length_fields.append(length_field)
        for child in xml:
            child_type = child.get("TypeName")
            if child_type and get_type_name(child_type) == "Bit":
                optional_fields.append(child.get("Name"))
        for child in xml:
            if not child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                continue
            if child.get("Name") in length_fields:
                continue
            if get_type_name(child.get("TypeName")) == "Bit":
                continue
            switch_field = child.get("SwitchField")
            if switch_field and switch_field in optional_fields:
                member_is_optional = True
            else:
                member_is_optional = False
            member_name = child.get("Name")
            member_name = member_name[:1].lower() + member_name[1:]
            member_type_name = get_type_name(child.get("TypeName"))
            member_type = types[member_type_name]
            is_array = True if child.get("LengthField") else False
            self.members.append(StructMember(member_name, member_type, is_array, member_is_optional))

        self.pointerfree = True
        for m in self.members:
            if m.is_array or m.is_optional or not m.member_type.pointerfree:
                self.pointerfree = False


class TypeDescription(object):
    def __init__(self, name, nodeid, namespaceid):
        self.name = name
        self.nodeid = nodeid
        self.namespaceid = namespaceid
        self.xmlEncodingId = "0"
        self.binaryEncodingId = "0"


class TypeParser():
    __metaclass__ = abc.ABCMeta

    def __init__(self, opaque_map, selected_types, no_builtin, outname, namespace):
        self.selected_types = []
        self.fh = None
        self.ff = None
        self.fc = None
        self.fe = None
        self.opaque_map = opaque_map
        self.selected_types = selected_types
        self.no_builtin = no_builtin
        self.outname = outname
        self.namespace = namespace
        self.types = OrderedDict()

    @staticmethod
    def merge_dicts(*dict_args):
        """
        Given any number of dicts, shallow copy and merge into a new dict,
        precedence goes to key value pairs in latter dicts.
        """
        result = {}
        for dictionary in dict_args:
            result.update(dictionary)
        return result

    def parseTypeDefinitions(self, outname, xmlDescription, namespace, addToTypes=None):
        def typeReady(element):
            "Are all member types defined?"
            for child in element:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                    childname = get_type_name(child.get("TypeName"))
                    if childname not in self.types and childname != "Bit":
                        return False
            return True

        def unknownTypes(element):
            "Return all unknown types"
            unknowns = []
            for child in element:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                    childname = get_type_name(child.get("TypeName"))
                    if childname not in self.types:
                        unknowns.append(childname)
            return unknowns

        def skipType(name):
            if name in excluded_types:
                return True
            if re.search("NodeId$", name) != None:
                return True
            return False

        def structWithOptionalFields(element):
            opt_fields = []
            for child in element:
                if child.tag != "{http://opcfoundation.org/BinarySchema/}Field":
                    continue
                typename = child.get("TypeName")
                if typename and get_type_name(typename) == "Bit":
                    if re.match(re.compile('.+Specified'), child.get("Name")):
                        opt_fields.append(child.get("Name"))
                    elif child.get("Name") == "Reserved1":
                        if len(opt_fields) + int(child.get("Length")) != 32:
                            return False
                        else:
                            break
                    else:
                        return False
                else:
                    return False
            for child in element:
                switchfield = child.get("SwitchField")
                if switchfield and switchfield in opt_fields:
                    opt_fields.remove(switchfield)
            return len(opt_fields) == 0

        def structWithBitFields(element):
            for child in element:
                typename = child.get("TypeName")
                if typename and get_type_name(typename) == "Bit":
                    return True
            return False

        snippets = {}
        for typeXml in etree.parse(xmlDescription).getroot():
            if not typeXml.get("Name"):
                continue
            name = typeXml.get("Name")
            snippets[name] = typeXml

        detectLoop = len(snippets) + 1
        while len(snippets) > 0:
            if detectLoop == len(snippets):
                name, typeXml = snippets.popitem()
                raise RuntimeError("Infinite loop detected or type not found while processing types " + name + ": unknonwn subtype " +
                                   str(unknownTypes(typeXml)) + ". If the unknown subtype is 'Bit', then maybe a struct with optional fields is defined wrong in the .bsd-file. If not, maybe you need to import additional types with the --import flag. " +
                                   "E.g. '--import==UA_TYPES#/path/to/deps/ua-nodeset/Schema/Opc.Ua.Types.bsd'")
            detectLoop = len(snippets)
            for name, typeXml in list(snippets.items()):
                if name in self.types or skipType(name):
                    del snippets[name]
                    continue
                if not typeReady(typeXml):
                    continue
                if structWithBitFields(typeXml) and not structWithOptionalFields(typeXml):
                    continue
                if name in builtin_types:
                    new_type = BuiltinType(name)
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedType":
                    new_type = EnumerationType(outname, typeXml, namespace)
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
                    new_type = OpaqueType(outname, typeXml, namespace, get_base_type_for_opaque(name)['name'])
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
                    new_type = StructType(outname, typeXml, namespace, self.types)
                else:
                    raise Exception("Type not known")

                self.types[name] = new_type
                if addToTypes is not None:
                    addToTypes[name] = new_type

                del snippets[name]

    @abc.abstractmethod
    def parse_types(self):
        pass

    def create_types(self):
        for builtin in builtin_types:
            self.types[builtin] = BuiltinType(builtin)

        for f in self.opaque_map:
            user_opaque_type_mapping.update(json.load(f))

        self.parse_types()

        # Read the selected data types
        arg_selected_types = self.selected_types
        self.selected_types = []
        for f in arg_selected_types:
            self.selected_types += list(filter(len, [line.strip() for line in f]))
        # Use all types if none are selected
        if len(self.selected_types) == 0:
            self.selected_types = self.types.keys()


class CSVBSDTypeParser(TypeParser):
    def __init__(self, opaque_map, selected_types, no_builtin, outname, namespace, import_bsd,
                 type_bsd, type_csv):
        TypeParser.__init__(self, opaque_map, selected_types, no_builtin, outname, namespace)
        self.typedescriptions = {}
        self.import_bsd = import_bsd
        self.type_bsd = type_bsd
        self.type_csv = type_csv
        self.types_imported = {}

    def parse_types(self):
        for i in self.import_bsd:
            (outname_import, file_import) = i.split("#")
            outname_import = outname_import.lower()
            if outname_import.startswith("ua_"):
                outname_import = outname_import[3:]
            self.parseTypeDefinitions(outname_import, file_import, self.namespace, addToTypes=self.types_imported)

        for f in self.type_bsd:
            self.parseTypeDefinitions(self.outname, f, self.namespace)

        for f in self.type_csv:
            self.typedescriptions = self.merge_dicts(self.typedescriptions,
                                                     self.parseTypeDescriptions(f, self.namespace))

    def parseTypeDescriptions(self, f, namespaceid):
        definitions = {}

        csvreader = csv.reader(f, delimiter=',')
        delay_init = []

        for row in csvreader:
            if len(row) < 3:
                continue
            if row[2] == "Object":
                # Check if node name ends with _Encoding_(DefaultXml|DefaultBinary) and store the node id in the
                # corresponding DataType
                m = re.match('(.*?)_Encoding_Default(Xml|Binary)$', row[0])
                if m:
                    baseType = m.group(1)
                    if baseType not in self.types:
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
            elif row[0] not in self.types:
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
