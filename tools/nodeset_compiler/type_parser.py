import codecs
import csv
import json
import xml.etree.ElementTree as etree
import xml.dom.minidom as dom
import copy
import re
from collections import OrderedDict

from .datatypes import QualifiedName, NodeId

try:
    from .opaque_type_mapping import get_base_type_for_opaque as get_base_type_for_opaque_ns0
except ImportError:
    from .nodeset_compiler.opaque_type_mapping import get_base_type_for_opaque as get_base_type_for_opaque_ns0

builtin_types = ["Boolean",  # 1
                 "SByte",    # 2
                 "Byte",     # 3
                 "Int16",    # 4
                 "UInt16",   # 5
                 "Int32",    # 6
                 "UInt32",   # 7
                 "Int64",    # 8
                 "UInt64",   # 9
                 "Float",    # 10
                 "Double",   # 11
                 "String",   # 12
                 "DateTime", # 13
                 "Guid",            # 14
                 "ByteString",      # 15
                 "XmlElement",      # 16
                 "NodeId",          # 17
                 "ExpandedNodeId",  # 18
                 "StatusCode",      # 19
                 "QualifiedName",   # 20
                 "LocalizedText",   # 21
                 "ExtensionObject", # 22
                 "DataValue",       # 23
                 "Variant",         # 24
                 "DiagnosticInfo"   # 25
                 ]

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
        raise TypeNotDefinedException(f"Unknown namespace: '{resultNs}'")
    if member_type_name not in types[resultNs]:
        raise TypeNotDefinedException(f"Unknown type: '{member_type_name}'")
    return types[resultNs][member_type_name]

def get_type_for_id(id, types):
    strid = str(id)
    for ns_url, ns_types in types.items():
        for t in ns_types.values():
            if str(t.nodeId) == strid:
                return t
    return None


def get_definition_fields(definition):
    """Return the direct Field children of a NodeSet2 Definition."""
    return [child for child in definition.childNodes
            if child.nodeType == child.ELEMENT_NODE and
            child.localName == "Field"]


# bsd is the xml definition from the .bsd file
# td is the xml "type-definition" from the nodeset-xml file
class Type:
    def __init__(self, outname, namespaceUri, bsd=None, td=None, name=None):
        self.outname = outname
        self.namespaceUri = namespaceUri
        self.pointerfree = False
        self.members = []
        self.description = ""
        self.nodeId = None
        self.binaryEncodingId = None
        self.xmlEncodingId = None
        self.baseTypeId = None
        self.isLocal = False
        if bsd is not None:
            self.name = bsd.get("Name")
        if bsd is not None:
            for child in bsd:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                    self.description = child.text
                    break
        if td is not None:
            self.name = QualifiedName(td.attributes["Name"].value).name
        if name is not None:
            self.name = name


class BuiltinType(Type):
    def __init__(self, name):
        Type.__init__(self, "types", "http://opcfoundation.org/UA/")
        self.name = name
        self.pointerfree = self.name in builtin_pointerfree
        idx = builtin_types.index(name)
        self.nodeId = NodeId(f"ns=0;i={idx+1}")


class EnumerationType(Type):
    def __init__(self, outname, namespace, bsd=None, td=None, name=None,
                 base_type=None):
        Type.__init__(self, outname, namespace, bsd=bsd, td=td, name=name)
        self.pointerfree = True
        self.elements = OrderedDict()
        self.isOptionSet = False
        self.lengthInBits = 32
        if bsd is not None:
            self.isOptionSet = bsd.get("IsOptionSet", "false") == "true"
            self.lengthInBits = int(bsd.get("LengthInBits", "32"))
        if td is not None:
            self.isOptionSet = td.getAttribute("IsOptionSet").lower() == "true"
            if self.isOptionSet and base_type is not None:
                widths = {"Byte": 8, "UInt16": 16,
                          "UInt32": 32, "UInt64": 64}
                if base_type.name not in widths:
                    raise RuntimeError(
                        f"OptionSet {self.name} has unsupported base type "
                        f"{base_type.name}")
                self.lengthInBits = widths[base_type.name]

        # default values for enumerations (encoded as int32):
        self.strDataType = "UA_Int32"
        self.strTypeKind = "UA_DATATYPEKIND_ENUM"
        self.strTypeIndex = "UA_TYPES_INT32"

        # special handling for OptionSet datatype (bitmask)
        if self.isOptionSet is True:
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
                    self.lengthInBits + "' is not supported")
        self.builtinTypeId = builtin_types.index(self.strDataType[3:]) + 1
        self.strBuiltinTypeKind = "UA_DATATYPEKIND_" + self.strDataType[3:].upper()

        # Get the defined values
        if bsd is not None:
            for child in bsd:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedValue":
                    self.elements[child.get("Name")] = child.get("Value")
        if td is not None:
            for field in get_definition_fields(td):
                if ("Name" not in field.attributes or
                        not field.getAttribute("Name")):
                    raise RuntimeError(
                        f"Type {self.name} has an enum field without a Name")
                if "Value" not in field.attributes:
                    raise RuntimeError(
                        f"Type {self.name} enum field {field.getAttribute('Name')} "
                        "has no Value")
                value_text = field.getAttribute("Value")
                try:
                    value = int(value_text)
                except ValueError as ex:
                    raise RuntimeError(
                        f"Type {self.name} enum field {field.getAttribute('Name')} "
                        f"has invalid Value {value_text}") from ex
                if self.isOptionSet and not 0 <= value < self.lengthInBits:
                    raise RuntimeError(
                        f"Type {self.name} option field {field.getAttribute('Name')} "
                        f"Value {value_text} is outside the UInt{self.lengthInBits} "
                        "bit range")
                if not self.isOptionSet and not -(2 ** 31) <= value < 2 ** 31:
                    raise RuntimeError(
                        f"Type {self.name} enum field {field.getAttribute('Name')} "
                        f"Value {value_text} is outside the Int32 range")
                # StructureDefinition encodes OptionSet values as bit
                # positions. BinarySchema uses the resulting bit masks.
                if self.isOptionSet:
                    value = 1 << value
                self.elements[field.getAttribute("Name")] = str(value)

    def set_base_type(self, base_type):
        """Use an OptionSet's unsigned-integer base type for its C width."""
        if not self.isOptionSet or base_type is None:
            return
        widths = {"Byte": 8, "UInt16": 16, "UInt32": 32, "UInt64": 64}
        if base_type.name not in widths:
            raise RuntimeError(
                f"OptionSet {self.name} has unsupported base type "
                f"{base_type.name}")
        self.lengthInBits = widths[base_type.name]
        self.strDataType = f"UA_{base_type.name}"
        self.strTypeKind = f"UA_DATATYPEKIND_{base_type.name.upper()}"
        self.strTypeIndex = f"UA_TYPES_{base_type.name.upper()}"
        self.builtinTypeId = builtin_types.index(base_type.name) + 1
        self.strBuiltinTypeKind = self.strTypeKind


class OpaqueType(Type):
    def __init__(self, outname, namespace, base_type, bsd=None):
        Type.__init__(self, outname, namespace, bsd=bsd)
        self.base_type = base_type


class StructMember:
    def __init__(self, name, member_type, is_array, is_optional,
                 original_name=None, value_rank=None, array_dimensions=None,
                 max_string_length=0, data_type_id=None):
        self.name = name
        self.original_name = original_name if original_name is not None else name
        self.member_type = member_type
        self.is_array = is_array
        self.is_optional = is_optional
        self.value_rank = value_rank if value_rank is not None else (1 if is_array else -1)
        self.array_dimensions = array_dimensions if array_dimensions is not None else []
        self.max_string_length = max_string_length
        self.data_type_id = (data_type_id if data_type_id is not None else
                             member_type.nodeId)


class StructType(Type):
    def __init__(self, outname, namespace, types, xmlNamespaces=None,
                 bsd=None, td=None, name=None):
        Type.__init__(self, outname, namespace, bsd=bsd, td=td, name=name)
        self.is_recursive = False
        self.is_union = False
        self.pointerfree = False
        self.own_members = []

        if bsd is not None:
            self._parse_bsd(bsd, types, xmlNamespaces)
        if td is not None:
            self._parse_td(td, types)

        self.update_pointerfree()

    def update_pointerfree(self):
        self.pointerfree = all(not m.is_array and not m.is_optional and
                               m.member_type.pointerfree for m in self.members)

    def _parse_td(self, td, types, types_by_id=None):
        if "IsUnion" in td.attributes and td.attributes["IsUnion"].value == "true":
            self.is_union = True

        fields = get_definition_fields(td)
        self.members = []
        for f in fields:
            if "Name" not in f.attributes or not f.getAttribute("Name"):
                raise RuntimeError(f"Type {self.name} has a field without a Name")
            original_name = f.attributes["Name"].value
            name = original_name[:1].lower() + original_name[1:]

            value_rank = -1
            if "ValueRank" in f.attributes:
                try:
                    value_rank = int(f.attributes["ValueRank"].value)
                except ValueError as ex:
                    raise RuntimeError(
                        f"Type {self.name} field {original_name} has invalid "
                        f"ValueRank {f.attributes['ValueRank'].value}") from ex
            if value_rank in (-2, -3):
                raise RuntimeError(
                    f"Type {self.name} field {original_name} has unsupported "
                    f"ValueRank {value_rank}: variable-rank fields cannot be "
                    f"represented by the fixed C structure layout")
            if value_rank < -1:
                raise RuntimeError(
                    f"Type {self.name} field {original_name} has invalid "
                    f"ValueRank {value_rank}")
            is_array = value_rank >= 0

            dimensions = []
            if "ArrayDimensions" in f.attributes:
                dimensions_text = f.attributes["ArrayDimensions"].value
                try:
                    dimension_values = dimensions_text.split(",")
                    if not dimensions_text or any(not v for v in dimension_values):
                        raise ValueError
                    dimensions = [int(v) for v in dimension_values]
                except ValueError as ex:
                    raise RuntimeError(
                        f"Type {self.name} field {original_name} has invalid "
                        f"ArrayDimensions {dimensions_text}") from ex
                if any(v < 0 for v in dimensions):
                    raise RuntimeError(
                        f"Type {self.name} field {original_name} has negative "
                        "ArrayDimensions")
            if not is_array and dimensions:
                raise RuntimeError(
                    f"Type {self.name} field {original_name} is scalar but "
                    "declares ArrayDimensions")
            if value_rank > 0 and dimensions and len(dimensions) != value_rank:
                raise RuntimeError(
                    f"Type {self.name} field {original_name} has ValueRank "
                    f"{value_rank} but {len(dimensions)} ArrayDimensions")

            max_string_length = 0
            if "MaxStringLength" in f.attributes:
                try:
                    max_string_length = int(f.attributes["MaxStringLength"].value)
                except ValueError as ex:
                    raise RuntimeError(
                        f"Type {self.name} field {original_name} has invalid "
                        "MaxStringLength") from ex
                if max_string_length < 0:
                    raise RuntimeError(
                        f"Type {self.name} field {original_name} has negative "
                        "MaxStringLength")

            is_optional = ("IsOptional" in f.attributes and
                           f.attributes["IsOptional"].value.lower() == "true")
            if self.is_union and is_optional:
                raise RuntimeError(
                    f"Type {self.name} is a union and cannot have optional "
                    f"field {original_name}")

            # DataType
            memberid = NodeId("ns=0;i=24")
            if "DataType" in f.attributes:
                memberid = NodeId(str(f.attributes["DataType"].value))
            if types_by_id is not None:
                member_type = types_by_id.get(str(memberid))
            else:
                member_type = get_type_for_id(memberid, types)
            if member_type is None:
                raise TypeNotDefinedException(
                    "Unknown member DataType %s in %s" %
                    (memberid, self.name))
            if member_type is self:
                if not is_array and not is_optional:
                    raise RuntimeError(
                        f"Type {self.name} contains itself as a non-indirect "
                        f"member {original_name}")
                self.is_recursive = True

            self.members.append(StructMember(
                name, member_type, is_array, is_optional,
                original_name=original_name, value_rank=value_rank,
                array_dimensions=dimensions,
                max_string_length=max_string_length,
                data_type_id=memberid))

        self.update_pointerfree()
        self.own_members = list(self.members)

    def _parse_bsd(self, bsd, types, xmlNamespaces):
        length_fields = []
        optional_fields = []
        switch_fields = []

        typename = type_aliases.get(bsd.get("Name"), bsd.get("Name"))

        bt = bsd.get("BaseType")
        self.is_union = bool(bt and get_type_name(bt)[1] == "Union")
        for child in bsd:
            length_field = child.get("LengthField")
            if length_field:
                length_fields.append(length_field)
        for child in bsd:
            switch_field = child.get("SwitchField")
            if switch_field:
                switch_fields.append(switch_field)
        for child in bsd:
            child_type = child.get("TypeName")
            if child_type and get_type_name(child_type)[1] == "Bit":
                optional_fields.append(child.get("Name"))
        for child in bsd:
            if not child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                continue
            if child.get("Name") in length_fields:
                continue
            if get_type_name(child.get("TypeName"))[1] == "Bit":
                continue
            if self.is_union and child.get("Name") in switch_fields:
                continue
            switch_field = child.get("SwitchField")
            member_is_optional = bool(switch_field and
                                      switch_field in optional_fields)
            member_name = child.get("Name")
            member_name = member_name[:1].lower() + member_name[1:]
            is_array = bool(child.get("LengthField"))

            member_type_name = get_type_name(child.get("TypeName"))[1]
            if member_type_name == typename: # If a type contains itself, use self as member_type
                if not is_array:
                    raise RuntimeError("Type " + typename +  " contains itself as a non-array member")
                member_type = self
                self.is_recursive = True
            else:
                member_type = get_type_for_name(child.get("TypeName"), types, xmlNamespaces)

            self.members.append(StructMember(
                member_name, member_type, is_array, member_is_optional,
                original_name=child.get("Name")))


class TypeParser():
    def __init__(self, opaque_map, selected_types, outname, namespaceIndexMap):
        self.opaque_map = opaque_map
        self.selected_types = selected_types
        self.outname = outname
        self.types = OrderedDict()
        self.types_by_id = {}
        self.namespaceIndexMap = namespaceIndexMap

        for builtin in builtin_types:
            self.insert_type(BuiltinType(builtin))

        for f in self.opaque_map:
            user_opaque_type_mapping.update(json.load(f))

        # Read the selected data types
        arg_selected_types = self.selected_types
        self.selected_types = []
        for f in arg_selected_types:
            self.selected_types += list(filter(len, [line.strip() for line in f]))

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

    @staticmethod
    def _definition_kind(td):
        fields = get_definition_fields(td)
        if len(fields) == 0:
            return None
        value_fields = ["Value" in field.attributes for field in fields]
        if all(value_fields):
            return "enum"
        if any(value_fields):
            raise RuntimeError(
                f"DataType Definition {td.getAttribute('Name')} mixes enum "
                "and structure fields")
        return "struct"

    def addTypesFromDefinitions(self, definitions, aliases=None):
        structs = []
        for definition in definitions:
            td, targetNamespace, nodeId = definition[:3]
            is_local = definition[3] if len(definition) > 3 else True
            base_id = definition[4] if len(definition) > 4 else None
            kind = self._definition_kind(td)
            if kind is None:
                continue
            if not is_local and str(nodeId) in self.types_by_id:
                continue
            if "Name" not in td.attributes:
                raise RuntimeError(f"DataType {nodeId} has a Definition without a Name")

            if kind == "enum":
                base_type = (self.types_by_id.get(str(base_id))
                             if base_id is not None else None)
                t = EnumerationType(self.outname, targetNamespace, td=td,
                                    base_type=base_type)
            else:
                name = QualifiedName(td.attributes["Name"].value).name
                t = StructType(self.outname, targetNamespace, self.types,
                               name=name)
                structs.append((t, td))
            t.nodeId = nodeId
            t.baseTypeId = base_id
            t.isLocal = is_local
            self.insert_type(t)

        # DataTypes without a Definition use the memory layout of their base
        # type. Index those aliases after the definition shells exist, so a
        # simple type can also derive from a custom type declared later.
        pending_aliases = list(aliases or [])
        changed = True
        while pending_aliases and changed:
            changed = False
            unresolved = []
            for node_id, base_id in pending_aliases:
                if str(node_id) in self.types_by_id:
                    changed = True
                    continue
                base_type = self.types_by_id.get(str(base_id))
                if base_type is None:
                    unresolved.append((node_id, base_id))
                    continue
                self.types_by_id[str(node_id)] = base_type
                changed = True
            pending_aliases = unresolved

        # OptionSet widths follow their unsigned integer base type. Definition
        # values have already been normalized from bit positions to masks.
        for namespace_types in self.types.values():
            for t in namespace_types.values():
                if (isinstance(t, EnumerationType) and t.isOptionSet and
                        t.baseTypeId is not None):
                    t.set_base_type(self.types_by_id.get(str(t.baseTypeId)))

        # All structure shells are now indexed. Resolve base structures before
        # derived structures, then flatten inherited fields for the generated
        # C ABI. Keep own_members separately for StructureDefinition metadata.
        structs_by_type = {id(t): (t, td) for t, td in structs}
        parsed = set()
        parsing = set()

        def parse_struct(t, td):
            key = id(t)
            if key in parsed:
                return
            if key in parsing:
                raise RuntimeError(
                    f"DataType inheritance cycle involving {t.name}")
            parsing.add(key)
            base = (self.types_by_id.get(str(t.baseTypeId))
                    if t.baseTypeId is not None else None)
            base_definition = structs_by_type.get(id(base))
            if base_definition is not None:
                parse_struct(*base_definition)
            t._parse_td(td, self.types, self.types_by_id)
            if isinstance(base, StructType):
                t.members = [copy.copy(member) for member in base.members] + t.members
                t.update_pointerfree()
            parsing.remove(key)
            parsed.add(key)

        for t, td in structs:
            parse_struct(t, td)

        # Pointer-freeness can depend on a structure declared later. Iterate
        # to a fixed point after all member links have been resolved.
        changed = True
        while changed:
            changed = False
            for t, _ in structs:
                old = t.pointerfree
                t.update_pointerfree()
                changed |= old != t.pointerfree

    def get_type_for_id(self, node_id):
        return self.types_by_id.get(str(node_id))

    def parseTypeDefinitions(self, outname, xmlDescription):
        def typeReady(element, types, xmlNamespaces):
            "Are all member types defined?"
            parentname = type_aliases.get(element.get("Name"), element.get("Name")) # If a type contains itself, declare that type as available
            for child in element:
                if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                    childname = get_type_name(child.get("TypeName"))[1]
                    if childname not in ("Bit", parentname):
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
        xmlDoc = etree.iterparse(xmlDescription, events=['start-ns'])
        xmlNamespaces = dict([node for _, node in xmlDoc])
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
                    new_type = EnumerationType(outname, targetNamespace, bsd=typeXml)
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
                    new_type = OpaqueType(outname, targetNamespace,
                                          get_base_type_for_opaque(name)['name'],
                                          bsd=typeXml)
                elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
                    try:
                        new_type = StructType(outname, targetNamespace, self.types, xmlNamespaces, bsd=typeXml)
                    except TypeNotDefinedException:
                        # Type is using other types which are not yet loaded, try later
                        continue
                else:
                    raise Exception("Type not known")

                self.insert_type(new_type)
                del snippets[name]

    def insert_type(self, t):
        if t.namespaceUri not in self.types:
            self.types[t.namespaceUri] = OrderedDict()

        if t.name in rename_types:
            t.name = rename_types[t.name]

        if t.name not in self.types[t.namespaceUri]:
            self.types[t.namespaceUri][t.name] = t
            if t.nodeId is not None:
                self.types_by_id[str(t.nodeId)] = t


class CSVBSDTypeParser(TypeParser):
    def __init__(self, opaque_map, selected_types, no_builtin, outname,
                 existing_bsd, type_bsd, type_csv, type_xml, namespaceIndexMap):
        TypeParser.__init__(self, opaque_map, selected_types, outname, namespaceIndexMap)
        self.no_builtin = no_builtin
        self.existing_bsd = existing_bsd # bsd files with existing types not printed again
        self.existing_types_array = set() # existing TYPE_ARRAY from existing_bsd
        self.type_bsd = type_bsd # bsd files with new types
        self.type_csv = type_csv # csv files with nodeids, etc.
        self.type_xml = type_xml # xml files with symbolicNames etc.
        self.existing_types = [] # existing types that shall not be printed
    def create_types(self):
        self._parse_types()

    def _parse_types(self):
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

    def _find_type_ns(self, typeName):
        """Find the namespace URI of a type by name, preferring the namespace
        that matches the current output file (self.outname).  This ensures CSV
        nodeIds are assigned to the correct spec's own type when the same type
        name also appears in an imported (dependency) namespace.

        Example
        -------
        Suppose Machinery/Jobs imports ISA95-JOBCONTROL which defines
        ``ProcessIrregularity`` (outname="types_isa95_jobcontrol"), and the
        MachineTool BSD also defines ``ProcessIrregularity`` for its own namespace
        (outname="types_machinetool").  When generating types_machinetool:

            self.outname == "types_machinetool"
            self._find_type_ns("ProcessIrregularity")
            # → returns the MachineTool namespace URI so that nodeId 62 from
            #   Opc.Ua.MachineTool.NodeIds.csv is stored on the MachineTool copy,
            #   not on the already-imported ISA95-JOBCONTROL copy.
        """
        for ns in self.types:
            if typeName in self.types[ns] and self.types[ns][typeName].outname == self.outname:
                return ns, typeName
        for ns in self.types:
            if typeName in self.types[ns]:
                return ns, typeName
        # Case-insensitive fallback: some companion specs (e.g. IREDES) have
        # a case mismatch between the BSD type name and the CSV/XML name.
        typeNameLower = typeName.lower()
        for ns in self.types:
            for t in self.types[ns]:
                if t.lower() == typeNameLower:
                    return ns, t
        return None, typeName

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
                    ns, key = self._find_type_ns(baseType)
                    if ns is not None:
                        self.types[ns][key].binaryEncodingId = row[1]

                # Check if node name ends with _Encoding_DefaultXml and store
                # the node id in the corresponding DataType
                m = re.match('(.*?)_Encoding_DefaultXml$', row[0])
                if m:
                    baseType = m.group(1)
                    ns, key = self._find_type_ns(baseType)
                    if ns is not None:
                        self.types[ns][key].xmlEncodingId = row[1]
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
            ns, key = self._find_type_ns(typeName)
            if ns is not None:
                self.types[ns][key].nodeId = row[1]
