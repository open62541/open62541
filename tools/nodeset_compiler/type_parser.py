import abc
import codecs
import csv
import json
import xml.etree.ElementTree as etree
import copy
import re
from collections import OrderedDict
import sys
import xml.dom.minidom as dom

if sys.version_info[0] >= 3:
    try:
        from opaque_type_mapping import get_base_type_for_opaque as get_base_type_for_opaque_ns0
    except ImportError:
        from nodeset_compiler.opaque_type_mapping import get_base_type_for_opaque as get_base_type_for_opaque_ns0
else:
    from opaque_type_mapping import get_base_type_for_opaque as get_base_type_for_opaque_ns0

builtin_types = ["Boolean", "SByte", "Byte", "Int16", "UInt16", "Int32", "UInt32",
                 "Int64", "UInt64", "Float", "Double", "String", "DateTime", "Guid",
                 "ByteString", "XmlElement", "NodeId", "ExpandedNodeId", "StatusCode",
                 "QualifiedName", "LocalizedText", "ExtensionObject", "DataValue",
                 "Variant", "DiagnosticInfo"]

builtin_pointerfree = ["Boolean", "SByte", "Byte", "Int16", "UInt16",
                       "Int32", "UInt32", "Int64", "UInt64", "Float", "Double",
                       "DateTime", "StatusCode", "Guid"]

# DataTypes that are ignored/not generated
excluded_types = [
    # NodeId Types
    "NodeIdType", "TwoByteNodeId", "FourByteNodeId", "NumericNodeId",
    "StringNodeId", "GuidNodeId", "ByteStringNodeId",
    # Node Types
    "InstanceNode", "TypeNode", "Node", "ObjectNode", "ObjectTypeNode", "VariableNode",
    "VariableTypeNode", "ReferenceTypeNode", "MethodNode", "ViewNode", "DataTypeNode"]

rename_types = {"NumericRange": "OpaqueNumericRange"}

# Type aliases
type_aliases = {"CharArray": "String"}

user_opaque_type_mapping = {}  # contains user defined opaque type mapping

class TypeNotDefinedException(Exception):
    pass

def get_base_type_for_opaque(name):
    if name in user_opaque_type_mapping:
        return user_opaque_type_mapping[name]
    else:
        return get_base_type_for_opaque_ns0(name)

def get_type_name(xml_type_name):
    [namespace, type_name] = xml_type_name.split(':', 1)
    return [namespace, type_aliases.get(type_name, type_name)]

def get_type_for_name(xml_type_name, types, xmlNamespaces):
    [member_type_name_ns, member_type_name] = get_type_name(xml_type_name)
    resultNs = xmlNamespaces[member_type_name_ns]
    if resultNs == 'http://opcfoundation.org/BinarySchema/':
        resultNs = 'http://opcfoundation.org/UA/'
    if resultNs not in types:
        raise TypeNotDefinedException("Unknown namespace: '{resultNs}'".format(
            resultNs=resultNs))
    if member_type_name not in types[resultNs]:
        raise TypeNotDefinedException("Unknown type: '{type}'".format(
            type=member_type_name))
    return types[resultNs][member_type_name]


class Type(object):
    def __init__(self, outname, xml, namespaceUri):
        self.name = None
        if xml is not None:
            self.name = xml.get("Name")
        self.outname = outname
        self.namespaceUri = namespaceUri
        self.pointerfree = False
        self.members = []
        self.description = ""
        self.nodeId = None
        self.binaryEncodingId = None
        if xml is not None:
            for child in xml:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                    self.description = child.text
                    break


class BuiltinType(Type):
    def __init__(self, name):
        Type.__init__(self, "types", None, "http://opcfoundation.org/UA/")
        self.name = name
        self.pointerfree = self.name in builtin_pointerfree


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
    def __init__(self, outname, xml, namespace, types, xmlNamespaces):
        Type.__init__(self, outname, xml, namespace)
        length_fields = []
        optional_fields = []
        switch_fields = []
        self.is_recursive = False

        typename = type_aliases.get(xml.get("Name"), xml.get("Name"))

        bt = xml.get("BaseType")
        self.is_union = True if bt and get_type_name(bt)[1] == "Union" else False
        for child in xml:
            length_field = child.get("LengthField")
            if length_field:
                length_fields.append(length_field)
        for child in xml:
            switch_field = child.get("SwitchField")
            if switch_field:
                switch_fields.append(switch_field)
        for child in xml:
            child_type = child.get("TypeName")
            if child_type and get_type_name(child_type)[1] == "Bit":
                optional_fields.append(child.get("Name"))
        for child in xml:
            if not child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                continue
            if child.get("Name") in length_fields:
                continue
            if get_type_name(child.get("TypeName"))[1] == "Bit":
                continue
            if self.is_union and child.get("Name") in switch_fields:
                continue
            switch_field = child.get("SwitchField")
            if switch_field and switch_field in optional_fields:
                member_is_optional = True
            else:
                member_is_optional = False
            member_name = child.get("Name")
            member_name = member_name[:1].lower() + member_name[1:]
            is_array = True if child.get("LengthField") else False

            member_type_name = get_type_name(child.get("TypeName"))[1]
            if member_type_name == typename: # If a type contains itself, use self as member_type
                if not is_array:
                    raise RuntimeError("Type " + typename +  " contains itself as a non-array member")
                member_type = self
                self.is_recursive = True
            else:
                member_type = get_type_for_name(child.get("TypeName"), types, xmlNamespaces)

            self.members.append(StructMember(member_name, member_type, is_array, member_is_optional))

        self.pointerfree = True
        for m in self.members:
            if m.is_array or m.is_optional or not m.member_type.pointerfree:
                self.pointerfree = False


class TypeParser():
    __metaclass__ = abc.ABCMeta

    def __init__(self, opaque_map, selected_types, no_builtin, outname, namespaceIndexMap):
        self.selected_types = []
        self.fh = None
        self.ff = None
        self.fc = None
        self.fe = None
        self.opaque_map = opaque_map
        self.selected_types = selected_types
        self.no_builtin = no_builtin
        self.outname = outname
        self.types = OrderedDict()
        self.namespaceIndexMap = namespaceIndexMap

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

    def parseTypeDefinitions(self, outname, xmlDescription):
        def typeReady(element, types, xmlNamespaces):
            "Are all member types defined?"
            parentname = type_aliases.get(element.get("Name"), element.get("Name")) # If a type contains itself, declare that type as available
            for child in element:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                    childname = get_type_name(child.get("TypeName"))[1]
                    if childname != "Bit" and childname != parentname:
                        try:
                            get_type_for_name(child.get("TypeName"), types, xmlNamespaces)
                        except TypeNotDefinedException:
                            # Type is using other types which are not yet loaded, try later
                            return False
            return True

        def unknownTypes(element, types, xmlNamespaces):
            "Return all unknown types (for debugging)"
            unknowns = []
            for child in element:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                    try:
                        get_type_for_name(child.get("TypeName"), types, xmlNamespaces)
                    except TypeNotDefinedException:
                        # Type is using other types which are not yet loaded, try later
                        unknowns.append(child.get("TypeName"))
            return unknowns

        def structWithOptionalFields(element):
            "Is this a structure with optional fields?"
            opt_fields = []
            for child in element:
                if child.tag != "{http://opcfoundation.org/BinarySchema/}Field":
                    continue
                typename = child.get("TypeName")
                if typename and get_type_name(typename)[1] == "Bit":
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
            "Is this a structure with bitfields?"
            for child in element:
                typename = child.get("TypeName")
                if typename and get_type_name(typename)[1] == "Bit":
                    return True
            return False

        snippets = OrderedDict()
        xmlDoc = etree.iterparse(
            xmlDescription, events=['start-ns']
        )
        xmlNamespaces = dict([
            node for _, node in xmlDoc
        ])
        targetNamespace = xmlDoc.root.get("TargetNamespace")
        for typeXml in xmlDoc.root:
            if not typeXml.get("Name"):
                continue
            name = typeXml.get("Name")
            snippets[name] = typeXml

        detectLoop = len(snippets) + 1
        while len(snippets) > 0:
            if detectLoop == len(snippets):
                name, typeXml = snippets.popitem()
                raise RuntimeError("Infinite loop detected or type not found while processing types " +
                                   name + ": unknonwn subtype " + str(unknownTypes(typeXml, self.types, xmlNamespaces)) +
                                   ". If the unknown subtype is 'Bit', then maybe a struct with " +
                                   "optional fields is defined wrong in the .bsd-file. If not, maybe " +
                                   "you need to import additional types with the --import flag. " +
                                   "E.g. '--import=UA_TYPES#/path/to/deps/ua-nodeset/Schema/" +
                                   "Opc.Ua.Types.bsd'")
            detectLoop = len(snippets)
            for name, typeXml in list(snippets.items()):
                if (targetNamespace in self.types and name in self.types[targetNamespace]) or name in excluded_types:
                    del snippets[name]
                    continue
                if not typeReady(typeXml, self.types, xmlNamespaces):
                    continue
                if structWithBitFields(typeXml) and not structWithOptionalFields(typeXml):
                    continue
                if name in builtin_types:
                    new_type = BuiltinType(name)
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedType":
                    new_type = EnumerationType(outname, typeXml, targetNamespace)
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
                    new_type = OpaqueType(outname, typeXml, targetNamespace,
                                          get_base_type_for_opaque(name)['name'])
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
                    try:
                        new_type = StructType(outname, typeXml, targetNamespace, self.types, xmlNamespaces)
                    except TypeNotDefinedException:
                        # Type is using other types which are not yet loaded, try later
                        continue
                else:
                    raise Exception("Type not known")

                self.insert_type(new_type)
                del snippets[name]

    @abc.abstractmethod
    def parse_types(self):
        pass

    def insert_type(self, typeObject):
        if typeObject.namespaceUri not in self.types:
            self.types[typeObject.namespaceUri] = OrderedDict()

        if typeObject.name in rename_types:
            typeObject.name = rename_types[typeObject.name]

        if typeObject.name not in self.types[typeObject.namespaceUri]:
            self.types[typeObject.namespaceUri][typeObject.name] = typeObject

    def create_types(self):
        for builtin in builtin_types:
            self.insert_type(BuiltinType(builtin))

        for f in self.opaque_map:
            user_opaque_type_mapping.update(json.load(f))

        self.parse_types()

        # Read the selected data types
        arg_selected_types = self.selected_types
        self.selected_types = []
        for f in arg_selected_types:
            self.selected_types += list(filter(len, [line.strip() for line in f]))


class CSVBSDTypeParser(TypeParser):
    def __init__(self, opaque_map, selected_types, no_builtin, outname,
                 existing_bsd, type_bsd, type_csv, type_xml, namespaceIndexMap):
        TypeParser.__init__(self, opaque_map, selected_types, no_builtin, outname, namespaceIndexMap)
        self.existing_bsd = existing_bsd # bsd files with existing types that shall not be printed again
        self.existing_types_array = set() # existing TYPE_ARRAY from existing_bsd
        self.type_bsd = type_bsd # bsd files with new types
        self.type_csv = type_csv # csv files with nodeids, etc.
        self.type_xml = type_xml # xml files with symbolicNames etc.
        self.existing_types = [] # existing types that shall not be printed

    def parse_types(self):
        # parse existing types
        for i in self.existing_bsd:
            (outname_import, file_import) = i.split("#")
            self.existing_types_array.add(outname_import)
            outname_import = outname_import.lower()
            if outname_import.startswith("ua_"):
                outname_import = outname_import[3:]
            self.parseTypeDefinitions(outname_import, file_import)

        # all types loaded up to now should be assumed as existing types and therefore
        # no code should be generated
        self.existing_types = copy.deepcopy(self.types)
        # if outname is types (generate typedefinitions for NS0), we still need the BuiltinType
        # therefore remove them from the existing array
        if self.outname == "types":
            for ns in self.types:
                for t in self.types[ns]:
                    if isinstance(self.types[ns][t], BuiltinType):
                       del self.existing_types[ns][t]

        # parse the new types
        for f in self.type_bsd:
            self.parseTypeDefinitions(self.outname, f)

        # create a lookup table with symbolicNames
        table = {}
        for f in self.type_xml:
            table = self.createSymbolicNameTable(f)

        # extend the type definitions with nodeids, etc. from the csv file
        for f in self.type_csv:
            self.parseTypeDescriptions(f, table)

    def createSymbolicNameTable(self, f):
        table = {}
        nodeset_base = open(f.name, "rb")
        fileContent = nodeset_base.read()
        # Remove BOM since the dom parser cannot handle it on python 3 windows
        if fileContent.startswith(codecs.BOM_UTF8):
            fileContent = fileContent.lstrip(codecs.BOM_UTF8)
        if sys.version_info >= (3, 0):
            fileContent = fileContent.decode("utf-8")

        # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
        fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\\g<1>\\g<2>\\g<3>>", fileContent)

        nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
        if len(nodesets) == 0 or len(nodesets) > 1:
            raise Exception("contains no or more then 1 nodeset")
        nodeset = nodesets[0]
        dataTypeNodes = nodeset.getElementsByTagName("UADataType")
        for nd in dataTypeNodes:
            if nd.hasAttribute("SymbolicName"):
                # Remove any digit and the colon
                result_string = re.sub(r'\d|:', '', nd.attributes["BrowseName"].nodeValue)
                table[nd.attributes["SymbolicName"].nodeValue] = result_string
        return table

    def parseTypeDescriptions(self, f, table):
        csvreader = csv.reader(f, delimiter=',')
        for row in csvreader:
            if len(row) < 3:
                continue
            if row[2] == "Object":
                # Check if node name ends with _Encoding_DefaultBinary and store
                # the node id in the corresponding DataType
                m = re.match('(.*?)_Encoding_DefaultBinary$', row[0])
                if m:
                    baseType = m.group(1)
                    for ns in self.types:
                        if baseType in self.types[ns]:
                            self.types[ns][baseType].binaryEncodingId = row[1]
                            break
                continue

            if row[2] != "DataType":
                continue

            typeName = row[0]
            if typeName == "BaseDataType":
                typeName = "Variant"
            elif typeName == "Structure":
                typeName = "ExtensionObject"
            if typeName in rename_types:
                typeName = rename_types[typeName]
            # check if typeName is a symbolicName and replace it with the browseName
            if typeName in table:
                typeName = table[typeName]
            for ns in self.types:
                if typeName in self.types[ns]:
                    self.types[ns][typeName].nodeId = row[1]
                    break
