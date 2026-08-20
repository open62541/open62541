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


def test_unsupported_variable_ranks():
    for value_rank in (-2, -3):
        document = minidom.parseString(
            '<Definition Name="1:BadRank">'
            f'<Field Name="Value" DataType="i=12" ValueRank="{value_rank}"/>'
            '</Definition>')
        parser = TypeParser({}, [], "bad_rank", {})
        try:
            parser.addTypesFromDefinitions([
                (document.documentElement, "urn:bad-rank",
                 NodeId("ns=1;i=7001"), True)])
        except RuntimeError as error:
            message = str(error)
            require(f"ValueRank {value_rank}" in message and
                    "fixed C structure layout" in message,
                    f"ValueRank {value_rank} diagnostic is not focused: "
                    f"{message}")
        else:
            raise AssertionError(f"ValueRank {value_rank} was accepted")


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
    require(abstract_option.strDataType == "UA_UInt32",
            "a direct subtype of the abstract OptionSet did not use UInt32")


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
            "StructureDefinition metadata duplicated inherited fields")


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
        ('ValueRank="2" ArrayDimensions="2"', "ValueRank 2",
         "1 ArrayDimensions"),
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


def main():
    test_unsupported_variable_ranks()
    test_enum_values_and_definition_validation()
    test_structure_inheritance()
    test_structure_validation()
    test_definition_alias_and_namespace_remapping()


if __name__ == "__main__":
    main()
