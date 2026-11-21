#!/usr/bin/env python3

import os
import sys
import textwrap
from pathlib import Path
from xml.dom import minidom


SOURCE_DIR = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, os.fspath(SOURCE_DIR / "tools"))

from nodeset_compiler.datatypes import NodeId
from nodeset_compiler.nodes import DataTypeNode
from nodeset_compiler.type_parser import EnumerationType, StructType, TypeParser


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def parse_definition(body, identifier=7001):
    document = minidom.parseString(textwrap.dedent(body))
    parser = TypeParser({}, [], "test", {})
    node_id = NodeId(f"ns=1;i={identifier}")
    parser.addTypesFromDefinitions([
        (document.documentElement, "urn:test", node_id, True)
    ])
    return parser.get_type_for_id(node_id)


def require_error(body, *parts):
    try:
        parse_definition(body)
    except Exception as error:
        message = str(error)
        require(all(part in message for part in parts),
                f"diagnostic is not focused: {message}")
    else:
        raise AssertionError("invalid definition was accepted")


def test_unsupported_ranks():
    for value_rank in (-3, -2, 0, 2, 3):
        document = minidom.parseString(
            '<Definition Name="1:BadRank">'
            f'<Field Name="Value" DataType="i=12" ValueRank="{value_rank}"/>'
            '</Definition>')
        parser = TypeParser({}, [], "bad_rank", {})
        parser.addTypesFromDefinitions([
            (document.documentElement, "urn:bad-rank",
             NodeId("ns=1;i=7001"), True)])
        datatype = parser.get_type_for_id(NodeId("ns=1;i=7001"))
        require(f"ValueRank {value_rank}" in datatype.unsupported,
                "unsupported layout was not marked for omission")


def test_enum_values_and_definition_validation():
    enumeration = parse_definition("""
        <Definition Name="1:SparseEnum">
          <Field Name="Negative" Value="-7"/>
          <Field Name="High" Value="42"/>
          <Field Name="Middle" Value="3"/>
        </Definition>
    """)
    require(isinstance(enumeration, EnumerationType),
            "enum definition did not produce an enumeration")
    require(list(enumeration.elements.items()) ==
            [("Negative", "-7"), ("High", "42"), ("Middle", "3")],
            "negative, sparse, or out-of-order enum values changed")

    require_error("""
        <Definition Name="1:Mixed">
          <Field Name="Enum" Value="1"/>
          <Field Name="Member" DataType="i=12"/>
        </Definition>
    """, "Mixed", "mixes enum and structure fields")
    require_error("""
        <Definition Name="1:BadEnum">
          <Field Name="Broken" Value="one"/>
        </Definition>
    """, "BadEnum", "Broken", "invalid Value one")
    require_error("""
        <Definition Name="1:BadEnum">
          <Field Name="TooLarge" Value="2147483648"/>
        </Definition>
    """, "BadEnum", "TooLarge", "outside the Int32 range")
    require_error("""
        <Definition Name="1:BadOption" IsOptionSet="true">
          <Field Name="Negative" Value="-1"/>
        </Definition>
    """, "BadOption", "Negative", "outside the UInt32 bit range")
    require_error("""
        <Definition Name="1:BadEnum"><Field Value="1"/></Definition>
    """, "BadEnum", "without a Name")

    option_document = minidom.parseString("""
        <Definition Name="1:NarrowOption" IsOptionSet="true">
          <Field Name="Low" Value="0"/>
          <Field Name="High" Value="15"/>
        </Definition>
    """)
    parser = TypeParser({}, [], "test", {})
    option_id = NodeId("ns=1;i=7002")
    parser.addTypesFromDefinitions([
        (option_document.documentElement, "urn:test", option_id, True,
         NodeId("ns=0;i=5"))
    ])
    option = parser.get_type_for_id(option_id)
    require(option.strDataType == "UA_UInt16",
            "OptionSet did not inherit its UInt16 representation")
    require(list(option.elements.values()) == ["1", "32768"],
            "OptionSet positions did not become UInt16 masks")

    abstract_option_id = NodeId("ns=1;i=7003")
    abstract_parser = TypeParser({}, [], "test", {})
    abstract_parser.addTypesFromDefinitions([
        (option_document.documentElement, "urn:test", abstract_option_id,
         True, NodeId("ns=0;i=12755"))
    ])
    abstract_option = abstract_parser.get_type_for_id(abstract_option_id)
    require(isinstance(abstract_option, StructType) and
            [m.member_type.name for m in abstract_option.members] == ["ByteString", "ByteString"],
            "an OptionSet subtype must retain Value/ValidBits ByteStrings")


def test_structure_inheritance():
    base_document = minidom.parseString("""
        <Definition Name="1:BaseRecord">
          <Field Name="BaseValue" DataType="i=6"/>
        </Definition>
    """)
    derived_document = minidom.parseString("""
        <Definition Name="1:DerivedRecord">
          <Field Name="DerivedValue" DataType="i=12"/>
        </Definition>
    """)
    base_id = NodeId("ns=1;i=7301")
    derived_id = NodeId("ns=1;i=7302")
    parser = TypeParser({}, [], "test", {})
    parser.addTypesFromDefinitions([
        (derived_document.documentElement, "urn:test", derived_id, True,
         base_id),
        (base_document.documentElement, "urn:test", base_id, True,
         NodeId("ns=0;i=22")),
    ])
    derived = parser.get_type_for_id(derived_id)
    require(isinstance(derived, StructType),
            "derived definition did not produce a structure")
    require([member.name for member in derived.members] ==
            ["baseValue", "derivedValue"],
            "inherited fields were not flattened into the C layout")
    require([member.name for member in derived.own_members] ==
            ["derivedValue"],
            "local NodeSet declarations duplicated inherited fields")


def test_structure_validation():
    require_error("""
        <Definition><Field Name="Value" DataType="i=12"/></Definition>
    """, "Definition without a Name")
    require_error("""
        <Definition Name="1:OptionalUnion" IsUnion="true">
          <Field Name="Value" DataType="i=12" IsOptional="true"/>
        </Definition>
    """, "OptionalUnion", "union", "optional field Value")
    require_error("""
        <Definition Name="1:MissingName"><Field DataType="i=12"/></Definition>
    """, "MissingName", "field without a Name")
    require_error("""
        <Definition Name="1:UnknownType">
          <Field Name="Value" DataType="ns=9;i=1234"/>
        </Definition>
    """, "Unknown member DataType", "ns=9;i=1234", "UnknownType")

    invalid_fields = [
        ('ValueRank="many"', "invalid ValueRank many"),
        ('ValueRank="-4"', "invalid ValueRank -4"),
        ('ValueRank="-1" ArrayDimensions="2"', "scalar", "ArrayDimensions"),
        ('ValueRank="1" ArrayDimensions="2,2"', "ValueRank 1",
         "2 ArrayDimensions"),
        ('ValueRank="1" ArrayDimensions="-1"', "negative ArrayDimensions"),
        ('ValueRank="1" ArrayDimensions="2,,3"',
         "invalid ArrayDimensions 2,,3"),
        ('MaxStringLength="-1"', "negative MaxStringLength"),
        ('MaxStringLength="large"', "invalid MaxStringLength"),
    ]
    for case in invalid_fields:
        attributes, *parts = case
        require_error(f"""
            <Definition Name="1:BadStructure">
              <Field Name="Value" DataType="i=12" {attributes}/>
            </Definition>
        """, "BadStructure", "Value", *parts)


def test_definition_alias_and_namespace_remapping():
    document = minidom.parseString("""
        <UADataType NodeId="ns=1;i=7201" BrowseName="1:Remapped">
          <Definition Name="1:Remapped">
            <Field Name="Aliased" DataType="StringAlias"/>
            <Field Name="Custom" DataType="ns=2;i=7202"/>
          </Definition>
        </UADataType>
    """)
    node = DataTypeNode(document.documentElement)
    node.replaceAliases({"StringAlias": "i=12"})
    node.replaceNamespaces({0: 0, 1: 3, 2: 4})
    fields = [child for child in node.dataTypeDefinition.childNodes
              if child.nodeType == child.ELEMENT_NODE and
              child.localName == "Field"]
    require(fields[0].getAttribute("DataType") == "ns=0;i=12",
            "alias in a definition field was not resolved")
    require(fields[1].getAttribute("DataType") == "ns=4;i=7202",
            "definition member namespace was not remapped")
    require(str(node.id) == "ns=3;i=7201",
            "datatype node namespace was not remapped")


def test_loader_definition_contracts():
    # The runtime loader uses this same fixture. NodeSet declarations contain
    # only local fields, while standard StructureDescriptions include all of them.
    fixture = SOURCE_DIR / "tests/nodeset-loader/nodesets/datatype_edge_cases.xml"
    document = minidom.parse(os.fspath(fixture))
    parser = TypeParser({}, [], "shared", {})
    definitions = []
    for node in document.getElementsByTagName("UADataType"):
        if node.getAttribute("NodeId") not in (
                "ns=1;i=6011", "ns=1;i=6012", "ns=1;i=6013"):
            continue
        parent = next(ref.firstChild.nodeValue for ref in
                      node.getElementsByTagName("Reference")
                      if ref.getAttribute("IsForward") == "false")
        definitions.append((node.getElementsByTagName("Definition")[0],
                            "urn:shared", NodeId(node.getAttribute("NodeId")),
                            True, NodeId(parent)))
    parser.addTypesFromDefinitions(definitions)
    grandchild = parser.get_type_for_id(NodeId("ns=1;i=6013"))
    require([member.name for member in grandchild.members] ==
            ["identifier", "measurement", "label"],
            "three-level inheritance expanded fields more than once")
    require([member.name for member in grandchild.own_members] == ["label"],
            "the local declaration contains inherited fields")

    matrix = next(node for node in document.getElementsByTagName("UADataType")
                  if node.getAttribute("NodeId") == "ns=1;i=7101")
    require("unsupported ValueRank 2" in
            parse_definition(matrix.getElementsByTagName("Definition")[0].toxml()).unsupported,
            "matrix layout was not marked for omission")


def test_alias_resolution_boundaries():
    holder = minidom.parseString('''<Definition Name="AliasHolder">
        <Field Name="Value" DataType="ns=1;i=7402"/>
    </Definition>''').documentElement
    parser = TypeParser({}, [], "test", {})
    holder_id = NodeId("ns=1;i=7403")
    aliases = [(NodeId("ns=1;i=7402"), NodeId("ns=1;i=7401")),
               (NodeId("ns=1;i=7401"), NodeId("i=12"))]
    parser.addTypesFromDefinitions([(holder, "urn:test", holder_id, True)], aliases)
    require(parser.get_type_for_id(holder_id).members[0].member_type.name == "String",
            "an alias chain did not resolve to its storage representation")

    base = minidom.parseString('''<Definition Name="BaseRecord">
        <Field Name="Identifier" DataType="i=7"/>
    </Definition>''').documentElement
    scenarios = [
        ([], [(NodeId("ns=1;i=7402"), NodeId("ns=1;i=7401")),
              (NodeId("ns=1;i=7401"), NodeId("ns=1;i=7402"))]),
        ([(base, "urn:test", NodeId("ns=1;i=7401"), True, NodeId("i=22"))],
         [(NodeId("ns=1;i=7402"), NodeId("ns=1;i=7401"))]),
    ]
    for definitions, aliases in scenarios:
        parser = TypeParser({}, [], "test", {})
        try:
            parser.addTypesFromDefinitions(
                definitions + [(holder, "urn:test", holder_id, True)], aliases)
        except Exception as error:
            require("Unknown member DataType ns=1;i=7402" in str(error),
                    f"unexpected unresolved-alias diagnostic: {error}")
        else:
            raise AssertionError("a cyclic or unresolved structured alias was accepted")


def main():
    test_unsupported_ranks()
    test_enum_values_and_definition_validation()
    test_structure_inheritance()
    test_loader_definition_contracts()
    test_alias_resolution_boundaries()
    test_structure_validation()
    test_definition_alias_and_namespace_remapping()


if __name__ == "__main__":
    main()
