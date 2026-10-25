#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
###    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)

import xml.dom.minidom as dom
import codecs
import re
import base64
import argparse

BSD_NAMESPACE = "http://opcfoundation.org/BinarySchema/"
UA_NAMESPACE = "http://opcfoundation.org/UA/"
BUILTIN_TYPES = {
    "i=1": "Boolean", "i=2": "SByte", "i=3": "Byte", "i=4": "Int16",
    "i=5": "UInt16", "i=6": "Int32", "i=7": "UInt32", "i=8": "Int64",
    "i=9": "UInt64", "i=10": "Float", "i=11": "Double", "i=12": "String",
    "i=13": "DateTime", "i=14": "Guid", "i=15": "ByteString",
    "i=16": "XmlElement", "i=17": "NodeId", "i=18": "ExpandedNodeId",
    "i=19": "StatusCode", "i=20": "QualifiedName", "i=21": "LocalizedText",
    "i=22": "ExtensionObject", "i=23": "DataValue", "i=24": "Variant",
    "i=25": "DiagnosticInfo"
}


def direct_children(node, name):
    return [child for child in node.childNodes
            if child.nodeType == child.ELEMENT_NODE and child.localName == name]


def first_child(node, name):
    children = direct_children(node, name)
    return children[0] if children else None


def node_text(node):
    return "".join(child.data for child in node.childNodes
                   if child.nodeType in (child.TEXT_NODE, child.CDATA_SECTION_NODE)).strip()


def local_name(name):
    return name.split(":", 1)[-1]


def namespace_index(node_id):
    match = re.match(r"ns=(\d+);", node_id)
    return int(match.group(1)) if match else 0


def inverse_subtype(node, aliases):
    references = first_child(node, "References")
    if references is None:
        return None
    for reference in direct_children(references, "Reference"):
        reference_type = aliases.get(reference.getAttribute("ReferenceType"),
                                     reference.getAttribute("ReferenceType"))
        if (reference_type == "i=45" and
                reference.getAttribute("IsForward").lower() == "false"):
            target = node_text(reference)
            return aliases.get(target, target)
    return None

###############################
# Parse the Command Line Input#
###############################

parser = argparse.ArgumentParser()
parser.add_argument('-x', '--xml',
                    metavar="<nodeSetXML>",
                    dest="xmlfile",
                    help='NodeSet XML file with nodes that shall be generated.')

parser.add_argument('outputFile',
                    metavar='<outputFile>',
                    help='output file w/o extension')

args = parser.parse_args()

# Extract the BSD Blob from the XML file.
nodeset_base = open(args.xmlfile, "rb")
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
variableNodes = nodeset.getElementsByTagName("UAVariable")
bsdFound = False
for nd in variableNodes:
    if (nd.hasAttribute("SymbolicName") and (re.match(r".*_BinarySchema", nd.attributes["SymbolicName"].nodeValue) or nd.attributes["SymbolicName"].nodeValue == "TypeDictionary_BinarySchema")) or (nd.hasAttribute("ParentNodeId") and not nd.hasAttribute("SymbolicName") and re.fullmatch(r"i=93", nd.attributes["ParentNodeId"].nodeValue)):
        type_content = nd.getElementsByTagName("Value")[0].getElementsByTagName("ByteString")[0]
        with open(args.outputFile, 'w') as f:
            f.write(base64.b64decode(type_content.firstChild.nodeValue).decode("utf-8"))
        bsdFound = True
    elif nd.hasAttribute("BrowseName") and nd.getAttribute("BrowseName").endswith("TypeDictionary"):
        references = nd.getElementsByTagName("Reference")
        for ref in references:
            if ref.getAttribute("ReferenceType") == "HasComponent" and ref.firstChild.nodeValue == "i=93":
                type_content = nd.getElementsByTagName("Value")[0].getElementsByTagName("ByteString")[0]
                with open(args.outputFile, 'w') as f:
                    f.write(base64.b64decode(type_content.firstChild.nodeValue).decode("utf-8"))
                bsdFound = True
                break

if bsdFound:
    raise SystemExit(0)


# Recent NodeSet files can carry complete DataType Definitions without the
# legacy embedded BSD dictionary. Convert these definitions into the BSD input
# already understood by the datatype generator.
aliases = {}
aliasesElement = first_child(nodeset, "Aliases")
if aliasesElement is not None:
    for alias in direct_children(aliasesElement, "Alias"):
        aliases[alias.getAttribute("Alias")] = node_text(alias)
aliasesById = {nodeId: name for name, nodeId in aliases.items()}

namespaceUris = []
namespaceElement = first_child(nodeset, "NamespaceUris")
if namespaceElement is not None:
    namespaceUris = [node_text(uri) for uri in direct_children(namespaceElement, "Uri")]

models = first_child(nodeset, "Models")
model = first_child(models, "Model") if models is not None else None
targetNamespace = model.getAttribute("ModelUri") if model is not None else ""
if not targetNamespace and namespaceUris:
    targetNamespace = namespaceUris[0]

dataTypes = {}
dataTypeNames = {}
for dataType in direct_children(nodeset, "UADataType"):
    definition = first_child(dataType, "Definition")
    if definition is None:
        continue
    nodeId = dataType.getAttribute("NodeId")
    name = (dataType.getAttribute("SymbolicName") or
            definition.getAttribute("SymbolicName") or
            definition.getAttribute("Name") or
            dataType.getAttribute("BrowseName"))
    dataTypes[nodeId] = dataType
    dataTypeNames[nodeId] = local_name(name)

if not targetNamespace or not dataTypes:
    raise SystemExit(0)


def resolve_type_name(value):
    nodeId = aliases.get(value, value)
    nsIndex = namespace_index(nodeId)
    name = dataTypeNames.get(nodeId,
                             aliasesById.get(nodeId,
                                             BUILTIN_TYPES.get(nodeId, local_name(value))))
    if nsIndex == 0:
        return "ua:" + name
    if nsIndex == 1:
        return "tns:" + name
    return "ns{}:{}".format(nsIndex, name)


def definition_fields(dataType, visited=None):
    """Return the memory-layout fields, including locally defined base types."""
    if visited is None:
        visited = set()
    nodeId = dataType.getAttribute("NodeId")
    if nodeId in visited:
        return []
    visited.add(nodeId)
    fields = []
    parentId = inverse_subtype(dataType, aliases)
    if parentId in dataTypes:
        fields.extend(definition_fields(dataTypes[parentId], visited))
    definition = first_child(dataType, "Definition")
    fields.extend(direct_children(definition, "Field"))
    return fields


document = dom.Document()
dictionary = document.createElementNS(BSD_NAMESPACE, "opc:TypeDictionary")
dictionary.setAttribute("xmlns:opc", BSD_NAMESPACE)
dictionary.setAttribute("xmlns:ua", UA_NAMESPACE)
dictionary.setAttribute("xmlns:tns", targetNamespace)
dictionary.setAttribute("TargetNamespace", targetNamespace)
dictionary.setAttribute("DefaultByteOrder", "LittleEndian")
for index, namespaceUri in enumerate(namespaceUris, start=1):
    if index > 1:
        dictionary.setAttribute("xmlns:ns{}".format(index), namespaceUri)
document.appendChild(dictionary)


def append_field(structure, name, typeName, valueRank, optional,
                 switchField=None, switchValue=None):
    lengthField = None
    if valueRank >= 0:
        lengthField = "NoOf" + name
        length = document.createElementNS(BSD_NAMESPACE, "opc:Field")
        length.setAttribute("Name", lengthField)
        length.setAttribute("TypeName", "opc:Int32")
        structure.appendChild(length)

    field = document.createElementNS(BSD_NAMESPACE, "opc:Field")
    field.setAttribute("Name", name)
    field.setAttribute("TypeName", typeName)
    if lengthField:
        field.setAttribute("LengthField", lengthField)
    if optional:
        field.setAttribute("SwitchField", name + "Specified")
    elif switchField:
        field.setAttribute("SwitchField", switchField)
        field.setAttribute("SwitchValue", str(switchValue))
    structure.appendChild(field)


for nodeId, dataType in dataTypes.items():
    definition = first_child(dataType, "Definition")
    ownFields = direct_children(definition, "Field")
    typeName = dataTypeNames[nodeId]
    isOptionSet = definition.getAttribute("IsOptionSet").lower() == "true"
    parentId = inverse_subtype(dataType, aliases)
    primitiveOptionSetWidths = {"i=3": 8, "i=5": 16, "i=7": 32, "i=9": 64}
    isPrimitiveOptionSet = isOptionSet and parentId in primitiveOptionSetWidths
    isStructureOptionSet = isOptionSet and parentId == "i=12755"
    isEnum = (isPrimitiveOptionSet or
              (not isStructureOptionSet and
               all(field.hasAttribute("Value") and
                   not field.hasAttribute("DataType") for field in ownFields)))

    if isEnum:
        generated = document.createElementNS(BSD_NAMESPACE, "opc:EnumeratedType")
        generated.setAttribute("Name", typeName)
        generated.setAttribute("LengthInBits",
                               str(primitiveOptionSetWidths.get(parentId, 32)))
        if isOptionSet:
            generated.setAttribute("IsOptionSet", "true")
        for field in ownFields:
            value = document.createElementNS(BSD_NAMESPACE, "opc:EnumeratedValue")
            value.setAttribute("Name", field.getAttribute("SymbolicName") or
                               field.getAttribute("Name"))
            value.setAttribute("Value", field.getAttribute("Value"))
            generated.appendChild(value)
        dictionary.appendChild(generated)
        continue

    if isStructureOptionSet:
        generated = document.createElementNS(BSD_NAMESPACE, "opc:StructuredType")
        generated.setAttribute("Name", typeName)
        generated.setAttribute("BaseType", "ua:OptionSet")
        append_field(generated, "Value", "ua:ByteString", -1, False)
        append_field(generated, "ValidBits", "ua:ByteString", -1, False)
        dictionary.appendChild(generated)
        continue

    fields = definition_fields(dataType)
    isUnion = definition.getAttribute("IsUnion").lower() == "true"
    generated = document.createElementNS(BSD_NAMESPACE, "opc:StructuredType")
    generated.setAttribute("Name", typeName)
    generated.setAttribute("BaseType", "ua:Union" if isUnion else "ua:ExtensionObject")
    if isUnion:
        switch = document.createElementNS(BSD_NAMESPACE, "opc:Field")
        switch.setAttribute("Name", "SwitchField")
        switch.setAttribute("TypeName", "opc:UInt32")
        generated.appendChild(switch)

    # BSD encodes all optional-field mask bits before the payload fields.
    optionalFields = [field for field in fields
                      if field.getAttribute("IsOptional").lower() == "true"]
    if len(optionalFields) > 32:
        raise RuntimeError("structures with more than 32 optional fields are not supported")
    for field in optionalFields:
        name = field.getAttribute("SymbolicName") or field.getAttribute("Name")
        bit = document.createElementNS(BSD_NAMESPACE, "opc:Field")
        bit.setAttribute("Name", name + "Specified")
        bit.setAttribute("TypeName", "opc:Bit")
        bit.setAttribute("Length", "1")
        generated.appendChild(bit)
    if optionalFields:
        reserved = document.createElementNS(BSD_NAMESPACE, "opc:Field")
        reserved.setAttribute("Name", "Reserved1")
        reserved.setAttribute("TypeName", "opc:Bit")
        reserved.setAttribute("Length", str(32 - len(optionalFields)))
        generated.appendChild(reserved)

    for index, field in enumerate(fields, start=1):
        name = field.getAttribute("SymbolicName") or field.getAttribute("Name")
        if field.getAttribute("AllowSubTypes").lower() == "true":
            fieldTypeName = "ua:ExtensionObject"
        else:
            fieldTypeName = resolve_type_name(field.getAttribute("DataType"))
        valueRank = int(field.getAttribute("ValueRank") or "-1")
        optional = field.getAttribute("IsOptional").lower() == "true"
        append_field(generated, name, fieldTypeName, valueRank, optional,
                     "SwitchField" if isUnion else None, index)
    dictionary.appendChild(generated)

with open(args.outputFile, "w", encoding="utf-8") as output:
    document.writexml(output, addindent="  ", newl="\n", encoding=None)
