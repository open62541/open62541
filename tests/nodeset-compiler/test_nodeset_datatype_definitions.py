#!/usr/bin/env python3

import os
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path


HERE = Path(__file__).resolve().parent
SOURCE_DIR = HERE.parent.parent
NODESET_COMPILER = SOURCE_DIR / "tools" / "nodeset_compiler" / "nodeset_compiler.py"
REDUCED_NODESET = SOURCE_DIR / "tools" / "schema" / "Opc.Ua.NodeSet2.Reduced.xml"
TYPE_DESCRIPTIONS = HERE / "typedescriptions.xml"

def run_compiler(output, *arguments):
    command = [sys.executable, os.fspath(NODESET_COMPILER)]
    command.extend(os.fspath(argument) for argument in arguments)
    command.append(os.fspath(output))
    return subprocess.run(command, capture_output=True, text=True)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def write_nodeset(path, body):
    path.write_text(textwrap.dedent(body), encoding="utf-8")


def nodeset_document(definitions):
    return f"""
        <UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
          <NamespaceUris>
            <Uri>urn:test</Uri>
          </NamespaceUris>
          <Models><Model ModelUri="urn:test"/></Models>
          <Aliases>
            <Alias Alias="HasSubtype">i=45</Alias>
            <Alias Alias="StringAlias">i=12</Alias>
          </Aliases>
          {definitions}
        </UANodeSet>
    """


def test_generated_static_types(tmpdir):
    output = tmpdir / "namespace_static_generated"
    types_output = tmpdir / "types_static"
    result = run_compiler(
        output, "--existing", REDUCED_NODESET, "--xml", TYPE_DESCRIPTIONS,
        "--types-array", "UA_TYPES", "--types-output", types_output)
    require(result.returncode == 0, result.stdout + result.stderr)

    namespace_header = output.with_suffix(".h").read_text(encoding="utf-8")
    namespace_source = output.with_suffix(".c").read_text(encoding="utf-8")
    types_header = Path(f"{types_output}_generated.h").read_text(
        encoding="utf-8")
    require('#include "types_static_generated.h"' in namespace_header,
            "generated namespace does not include its static datatype API")
    require("UA_TYPES_STATIC_COUNT" in types_header and
            "UA_TYPES_STATIC[UA_TYPES_STATIC_COUNT]" in types_header,
            "inline XML did not generate the public datatype array")
    require("UA_Address_new" in types_header,
            "inline XML did not generate datatype helper functions")
    require("customUA_TYPES_STATIC" in namespace_source,
            "generated static datatype array is not registered")
    require("UA_Server_addDataTypeFromDescription" not in namespace_source,
            "static datatype generation still constructs types from descriptions")


def test_static_types_after_definitionless_dependency(tmpdir):
    dependency = tmpdir / "definitionless.xml"
    write_nodeset(dependency, nodeset_document("""
        <UADataType NodeId="ns=1;i=6001" BrowseName="1:NoDefinition">
          <DisplayName>NoDefinition</DisplayName>
          <References><Reference ReferenceType="HasSubtype"
            IsForward="false">i=22</Reference></References>
        </UADataType>
        <UADataType NodeId="ns=1;i=6002" BrowseName="1:EmptyDefinition">
          <DisplayName>EmptyDefinition</DisplayName>
          <References><Reference ReferenceType="HasSubtype"
            IsForward="false">i=22</Reference></References>
          <Definition Name="1:EmptyDefinition"/>
        </UADataType>
    """))
    output = tmpdir / "namespace_after_empty_generated"
    types_output = tmpdir / "types_after_empty"
    result = run_compiler(
        output, "--existing", REDUCED_NODESET, "--existing", dependency,
        "--xml", TYPE_DESCRIPTIONS, "--types-array", "UA_TYPES",
        "--types-array", "UA_TYPES_EMPTY", "--types-output", types_output)
    require(result.returncode == 0, result.stdout + result.stderr)

    types_header = Path(f"{types_output}_generated.h").read_text(
        encoding="utf-8")
    require("UA_TYPES_AFTER_EMPTY_COUNT 7" in types_header,
            "an empty or definition-less dependency captured the local "
            "datatype array")
    require("UA_TYPES_AFTER_EMPTY_ADDRESS" in types_header,
            "local definitions disappeared after an empty dependency")


def test_imported_type_not_redeclared(tmpdir):
    dependency = tmpdir / "dependency.xml"
    dependency_bsd = tmpdir / "dependency.bsd"
    local = tmpdir / "local.xml"
    write_nodeset(dependency_bsd, """
        <opc:TypeDictionary
          xmlns:opc="http://opcfoundation.org/BinarySchema/"
          xmlns:ua="http://opcfoundation.org/UA/"
          xmlns:tns="urn:datatype-dependency"
          TargetNamespace="urn:datatype-dependency">
          <opc:StructuredType Name="Nested" BaseType="ua:ExtensionObject">
            <opc:Field Name="Value" TypeName="opc:String"/>
          </opc:StructuredType>
          <opc:StructuredType Name="DepRecord" BaseType="ua:ExtensionObject">
            <opc:Field Name="Nested" TypeName="tns:Nested"/>
          </opc:StructuredType>
        </opc:TypeDictionary>
    """)
    write_nodeset(dependency, """
        <UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
          <NamespaceUris>
            <Uri>urn:datatype-dependency</Uri>
          </NamespaceUris>
          <Models><Model ModelUri="urn:datatype-dependency"/></Models>
          <Aliases><Alias Alias="HasSubtype">i=45</Alias></Aliases>
          <UADataType NodeId="ns=1;i=5000" BrowseName="1:Nested">
            <DisplayName>Nested</DisplayName>
            <References>
              <Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference>
            </References>
            <Definition Name="1:Nested">
              <Field DataType="i=12" Name="Value"/>
            </Definition>
          </UADataType>
          <UADataType NodeId="ns=1;i=5001" BrowseName="1:DepRecord">
            <DisplayName>DepRecord</DisplayName>
            <References>
              <Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference>
            </References>
            <Definition Name="1:DepRecord">
              <Field DataType="ns=1;i=5000" Name="Nested"/>
            </Definition>
          </UADataType>
        </UANodeSet>
    """)
    write_nodeset(local, """
        <UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
          <NamespaceUris>
            <Uri>urn:datatype-dependency</Uri>
            <Uri>urn:datatype-local</Uri>
          </NamespaceUris>
          <Models>
            <Model ModelUri="urn:datatype-local">
              <RequiredModel ModelUri="urn:datatype-dependency"/>
            </Model>
          </Models>
          <Aliases><Alias Alias="HasSubtype">i=45</Alias></Aliases>
          <UADataType NodeId="ns=2;i=6001" BrowseName="2:LocalRecord">
            <DisplayName>LocalRecord</DisplayName>
            <References>
              <Reference ReferenceType="HasSubtype" IsForward="false">ns=1;i=5001</Reference>
            </References>
            <Definition Name="2:LocalRecord">
              <Field DataType="i=12" Name="Extra"/>
            </Definition>
          </UADataType>
          <UADataType NodeId="ns=2;i=6002" BrowseName="2:DepRecord">
            <DisplayName>DepRecord</DisplayName>
            <References>
              <Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference>
            </References>
            <Definition Name="2:DepRecord">
              <Field DataType="ns=1;i=5000" Name="Nested"/>
            </Definition>
          </UADataType>
        </UANodeSet>
    """)

    output = tmpdir / "local_generated"
    types_output = tmpdir / "types_local"
    result = run_compiler(
        output, "--existing", REDUCED_NODESET, "--existing", dependency,
        "--xml", local, "--bsd", dependency_bsd,
        "--types-array", "UA_TYPES", "--types-array", "UA_TYPES_DEPENDENCY",
        "--types-output", types_output)
    require(result.returncode == 0, result.stdout + result.stderr)
    header = Path(f"{types_output}_generated.h").read_text(encoding="utf-8")
    source = Path(f"{types_output}_generated.c").read_text(encoding="utf-8")
    require('#include "types_dependency_generated.h"' in header,
            "dependency type header was not included")
    require("UA_Nested nested;" in header,
            "local inheritance did not retain its imported base member")
    require("} UA_DepRecord;" not in header,
            "an identical cross-namespace dependency typedef was emitted again")
    require("UA_TYPES_LOCAL_DEPRECORD" in header,
            "cross-namespace duplicate is missing from the local type array")
    require("UA_TYPES_DEPENDENCY[" in source,
            "local static layout does not reference the dependency array")
    require("UA_TYPES_DEPENDENCY_NESTED" in source,
            "inherited BSD members retained a stale datatype-array owner")


def test_mutual_recursion_rejected(tmpdir):
    nodeset = tmpdir / "mutual.xml"
    write_nodeset(nodeset, """
        <UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
          <NamespaceUris>
            <Uri>urn:mutual-recursion</Uri>
          </NamespaceUris>
          <Models><Model ModelUri="urn:mutual-recursion"/></Models>
          <Aliases><Alias Alias="HasSubtype">i=45</Alias></Aliases>
          <UADataType NodeId="ns=1;i=8001" BrowseName="1:RecursiveA">
            <DisplayName>RecursiveA</DisplayName>
            <References>
              <Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference>
            </References>
            <Definition Name="1:RecursiveA">
              <Field DataType="ns=1;i=8002" ValueRank="1" Name="Values"/>
            </Definition>
          </UADataType>
          <UADataType NodeId="ns=1;i=8002" BrowseName="1:RecursiveB">
            <DisplayName>RecursiveB</DisplayName>
            <References>
              <Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference>
            </References>
            <Definition Name="1:RecursiveB">
              <Field DataType="ns=1;i=8001" ValueRank="1" Name="Values"/>
            </Definition>
          </UADataType>
        </UANodeSet>
    """)
    result = run_compiler(
        tmpdir / "mutual_generated", "--existing", REDUCED_NODESET,
        "--xml", nodeset, "--types-array", "UA_TYPES")
    require(result.returncode != 0, "mutually recursive types were accepted")
    require("Mutually recursive DataTypes" in result.stdout + result.stderr,
            "mutual-recursion rejection did not have a focused diagnostic")


def test_empty_missing_and_differently_named_definitions(tmpdir):
    nodeset = tmpdir / "definition_edges.xml"
    write_nodeset(nodeset, nodeset_document("""
        <UADataType NodeId="ns=1;i=7101" BrowseName="1:NoDefinition">
          <DisplayName>NoDefinition</DisplayName>
          <References><Reference ReferenceType="HasSubtype"
            IsForward="false">i=22</Reference></References>
        </UADataType>
        <UADataType NodeId="ns=1;i=7102" BrowseName="1:EmptyDefinition">
          <DisplayName>EmptyDefinition</DisplayName>
          <References><Reference ReferenceType="HasSubtype"
            IsForward="false">i=22</Reference></References>
          <Definition Name="1:EmptyDefinition"/>
        </UADataType>
        <UADataType NodeId="ns=1;i=7103" BrowseName="1:BrowseType">
          <DisplayName>BrowseType</DisplayName>
          <References><Reference ReferenceType="HasSubtype"
            IsForward="false">i=22</Reference></References>
          <Definition Name="1:DefinitionType">
            <Field Name="AliasedValue" DataType="StringAlias"/>
          </Definition>
        </UADataType>
        <UADataType NodeId="ns=1;i=7104" BrowseName="1:SimpleString">
          <DisplayName>SimpleString</DisplayName>
          <References><Reference ReferenceType="HasSubtype"
            IsForward="false">i=12</Reference></References>
        </UADataType>
        <UADataType NodeId="ns=1;i=7105" BrowseName="1:AliasHolder">
          <DisplayName>AliasHolder</DisplayName>
          <References><Reference ReferenceType="HasSubtype"
            IsForward="false">i=22</Reference></References>
          <Definition Name="1:AliasHolder">
            <Field Name="Value" DataType="ns=1;i=7104"/>
          </Definition>
        </UADataType>
    """))
    output = tmpdir / "definition_edges"
    types_output = tmpdir / "types_definition_edges"
    result = run_compiler(output, "--existing", REDUCED_NODESET,
                          "--xml", nodeset, "--types-array", "UA_TYPES",
                          "--types-output", types_output)
    require(result.returncode == 0, result.stdout + result.stderr)
    header = Path(f"{types_output}_generated.h").read_text(encoding="utf-8")
    require("UA_NoDefinition" not in header and
            "UA_EmptyDefinition" not in header,
            "definition-less datatype unexpectedly emitted a C declaration")
    require("} UA_DefinitionType;" in header,
            "Definition Name is not used for the generated C type")
    require("UA_String aliasedValue;" in header,
            "definition field alias was not resolved")
    require("UA_String value;" in header,
            "custom simple type did not use its builtin C representation")


def main():
    with tempfile.TemporaryDirectory() as directory:
        tmpdir = Path(directory)
        test_generated_static_types(tmpdir)
        test_static_types_after_definitionless_dependency(tmpdir)
        test_imported_type_not_redeclared(tmpdir)
        test_mutual_recursion_rejected(tmpdir)
        test_empty_missing_and_differently_named_definitions(tmpdir)


if __name__ == "__main__":
    main()
