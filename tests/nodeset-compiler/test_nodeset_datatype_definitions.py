#!/usr/bin/env python3

import os
import base64
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
    internal_header = Path(f"{types_output}_generated_internal.h").read_text(encoding="utf-8")
    types_source = Path(f"{types_output}_generated.c").read_text(encoding="utf-8")
    for header in (types_header, namespace_header):
        require("UA_GeneratedTypeSource" not in header and "_SOURCE" not in header,
                "generated application header exposes registration metadata")
        require("generated_internal.h" not in header and "internal/generated_types.h" not in header,
                "generated application header includes implementation support")
    require("UA_GeneratedTypeSource" not in internal_header and "UA_GeneratedTypes_" not in internal_header,
            "private header exposes the loader's metadata type or functions")
    require("UA_GeneratedTypeSource" not in types_source,
            "datatype source depends on the loader's metadata type")
    require("UA_DataType_toDescription" in namespace_source and "UA_DataType_fromDescription" in namespace_source,
            "conversion support is missing from the namespace C file")
    require("UA_GeneratedTypes_match" not in namespace_source,
            "generated loader still compares native layouts at runtime")
    require("UA_Server_addDataType(server, UA_NODEID_NULL, &type)" in namespace_source,
            "generated copy mode does not delegate registration to the SDK")
    require("UA_DataTypeArray *array =" not in namespace_source,
            "generated copy mode still manages descriptor storage")
    require("UA_Boolean *ready" not in namespace_source,
            "generated loader still schedules inline dependencies at runtime")
    require('#include "types_static_generated_internal.h"' in types_source and
            '#include "types_static_generated_internal.h"' in namespace_source,
            "generated C does not include its private registration interface")
    require("UA_GeneratedTypes_register(server, &UA_TYPES_STATIC_source" in namespace_source,
            "generated static datatype array is not registered")
    require("UA_Server_addDataTypeFromDescription" not in namespace_source,
            "static datatype generation still constructs types from descriptions")


def test_pinned_xml_types(tmpdir):
    output = tmpdir / "namespace_pinned_generated"
    types = tmpdir / "types_pinned"
    args = ("--existing", REDUCED_NODESET, "--xml", TYPE_DESCRIPTIONS,
            "--types-array", "UA_TYPES", "--types-output", types)
    result = run_compiler(output, *args,
        "--namespaceMap=2:http://model.o6-automation.com",
        "--namespaceMap=5:urn:unrelated-server-namespace")
    require(result.returncode == 0, result.stdout + result.stderr)
    header = Path(f"{types}_generated.h").read_text()
    source = Path(f"{types}_generated.c").read_text()
    require("extern UA_EXPORT const UA_DataType" in header, "pinned XML array is writable")
    require("UA_TYPES_PINNED_NAMESPACE_MAP" in header, "XML pin map was not forwarded")
    require("_ORIGIN" not in source, "duplicate origin array was generated")
    result = run_compiler(output, *args, "--namespaceMap=5:urn:unrelated-server-namespace")
    require(result.returncode != 0 and "incomplete" in result.stdout + result.stderr,
            "incomplete XML namespace pin map was accepted")


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


def test_mutual_recursion(tmpdir):
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
    output = tmpdir / "mutual_generated"
    types_output = tmpdir / "types_mutual"
    result = run_compiler(
        output, "--existing", REDUCED_NODESET,
        "--xml", nodeset, "--types-array", "UA_TYPES",
        "--types-output", types_output)
    require(result.returncode != 0 and
            "Mutually recursive datatype dependencies" in result.stdout + result.stderr,
            "mutual recursion must fail during generation")
    # Replacing the array edges with inline fields creates an infinite layout.
    nodeset.write_text(nodeset.read_text().replace('ValueRank="1"', ''), encoding="utf-8")
    result = run_compiler(output, "--existing", REDUCED_NODESET,
                          "--xml", nodeset, "--types-array", "UA_TYPES")
    require(result.returncode != 0, "inline dependency cycle was accepted")
    require("cycle of inline members" in result.stdout + result.stderr,
            "inline-cycle rejection did not have a focused diagnostic")


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
            <Field Name="aliasedValue" DataType="StringAlias"/>
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
    source = Path(f"{types_output}_generated.c").read_text()
    require('UA_TYPENAME("BrowseType")' in source,
            "XML datatype metadata used the C declaration name instead of the BrowseName")
    require('UA_TYPENAME("aliasedValue")' in source,
            "datatype metadata changed the case of an XML field name")
    require("typedef UA_String UA_SimpleString;" in header and "UA_SimpleString value;" in header,
            "custom simple type did not retain its builtin C representation")
    require("UA_TYPES_DEFINITION_EDGES_SIMPLESTRING" in source,
            "custom simple type is missing its own metadata descriptor")


def test_standard_structure_members(tmpdir):
    nodeset = tmpdir / "standard_fields.xml"
    write_nodeset(nodeset, nodeset_document("""
        <UADataType NodeId="ns=1;i=7110" BrowseName="1:StandardHolder">
          <DisplayName>StandardHolder</DisplayName>
          <References><Reference ReferenceType="i=45" IsForward="false">i=22</Reference></References>
          <Definition Name="1:StandardHolder">
            <Field Name="Units" DataType="i=887"/>
            <Field Name="Token" DataType="i=322"/>
          </Definition>
        </UADataType>
    """))
    types_output = tmpdir / "types_standard"
    result = run_compiler(tmpdir / "standard_generated", "--existing", REDUCED_NODESET,
                          "--xml", nodeset, "--types-array", "UA_TYPES",
                          "--types-output", types_output)
    require(result.returncode == 0, result.stdout + result.stderr)
    header = Path(f"{types_output}_generated.h").read_text(encoding="utf-8")
    source = Path(f"{types_output}_generated.c").read_text(encoding="utf-8")
    require("UA_EUInformation units;" in header, "standard structure was not resolved")
    require("UA_UserNameIdentityToken token;" in header, "inherited standard type was not resolved")
    require("UA_TYPES[UA_TYPES_EUINFORMATION]" in source,
            "standard structure member does not reference UA_TYPES")


def test_unsupported_layouts_keep_namespace(tmpdir):
    nodeset = tmpdir / "unsupported.xml"
    write_nodeset(nodeset, nodeset_document("""
        <UADataType NodeId="ns=1;i=8001" BrowseName="1:Flexible">
          <DisplayName>Flexible</DisplayName>
          <References><Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference></References>
          <Definition Name="Flexible"><Field Name="Value" DataType="i=12" ValueRank="-2"/></Definition>
        </UADataType>
        <UADataType NodeId="ns=1;i=8002" BrowseName="1:Direct">
          <DisplayName>Direct</DisplayName>
          <References><Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference></References>
          <Definition Name="Direct"><Field Name="Value" DataType="ns=1;i=8001"/></Definition>
        </UADataType>
        <UADataType NodeId="ns=1;i=8003" BrowseName="1:Polymorphic">
          <DisplayName>Polymorphic</DisplayName>
          <References><Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference></References>
          <Definition Name="Polymorphic"><Field Name="Value" DataType="ns=1;i=8001" AllowSubTypes="true"/></Definition>
        </UADataType>
    """))
    output = tmpdir / "namespace_unsupported_generated"
    types = tmpdir / "types_unsupported"
    result = run_compiler(output, "--existing", REDUCED_NODESET, "--xml", nodeset,
                          "--types-array", "UA_TYPES", "--types-output", types)
    require(result.returncode == 0, result.stdout + result.stderr)
    require("WARNING" in result.stdout and "Omitting native datatype Flexible" in result.stdout
            and "Omitting native datatype Direct" in result.stdout,
            "omitted native layouts did not produce warnings")
    header = Path(f"{types}_generated.h").read_text()
    require("UA_Flexible" not in header and "UA_Direct" not in header,
            "unsupported native descriptors were generated")
    require("UA_Polymorphic" in header and "UA_ExtensionObject value" in header,
            "polymorphic references to unsupported layouts were lost")
    namespace = output.with_suffix(".c").read_text()
    require(all(str(i) in namespace for i in (8001, 8002, 8003)),
            "namespace generation discarded an unsupported datatype node")


def test_embedded_dictionary_does_not_override_xml(tmpdir):
    bsd = """<opc:TypeDictionary xmlns:opc="http://opcfoundation.org/BinarySchema/"
        TargetNamespace="urn:test"><opc:StructuredType Name="BaseRecord">
        <opc:Field Name="Value" TypeName="opc:ByteString"/>
        </opc:StructuredType></opc:TypeDictionary>"""
    schema = tmpdir / "embedded.bsd"
    schema.write_text(bsd)
    encoded = base64.b64encode(bsd.encode()).decode()
    dependency = tmpdir / "embedded_dependency.xml"
    write_nodeset(dependency, nodeset_document(f"""
        <UADataType NodeId="ns=1;i=8101" BrowseName="1:BaseRecord">
          <DisplayName>BaseRecord</DisplayName>
          <References><Reference ReferenceType="HasSubtype" IsForward="false">i=22</Reference></References>
          <Definition Name="BaseRecord"><Field Name="Value" DataType="i=12"/></Definition>
        </UADataType>
        <UAVariable NodeId="ns=1;i=8199" BrowseName="1:Dictionary"
                    SymbolicName="Dictionary_BinarySchema" DataType="i=15">
          <DisplayName>Dictionary</DisplayName>
          <References><Reference ReferenceType="i=40">i=63</Reference></References>
          <Value><ByteString>{encoded}</ByteString></Value>
        </UAVariable>
    """))
    local = tmpdir / "embedded_local.xml"
    write_nodeset(local, nodeset_document("""
        <UADataType NodeId="ns=1;i=8102" BrowseName="1:DerivedRecord">
          <DisplayName>DerivedRecord</DisplayName>
          <References><Reference ReferenceType="HasSubtype" IsForward="false">ns=1;i=8101</Reference></References>
          <Definition Name="DerivedRecord"><Field Name="Extra" DataType="i=7"/></Definition>
        </UADataType>
    """))
    for explicit in (False, True):
        types = tmpdir / ("types_explicit" if explicit else "types_embedded")
        arguments = ["--existing", REDUCED_NODESET, "--existing", dependency,
                     "--xml", local, "--types-array", "UA_TYPES",
                     "--types-array", "UA_TYPES_DEPENDENCY", "--types-output", types]
        if explicit:
            arguments += ["--bsd", schema]
        result = run_compiler(tmpdir / (types.name + "_namespace"), *arguments)
        require(result.returncode == 0, result.stdout + result.stderr)
        header = Path(f"{types}_generated.h").read_text()
        expected = "UA_ByteString value;" if explicit else "UA_String value;"
        require(expected in header,
                "dependency used the wrong frontend's datatype representation")


def main():
    with tempfile.TemporaryDirectory() as directory:
        tmpdir = Path(directory)
        test_embedded_dictionary_does_not_override_xml(tmpdir)
        test_unsupported_layouts_keep_namespace(tmpdir)
        test_generated_static_types(tmpdir)
        test_pinned_xml_types(tmpdir)
        test_standard_structure_members(tmpdir)
        test_static_types_after_definitionless_dependency(tmpdir)
        test_imported_type_not_redeclared(tmpdir)
        test_mutual_recursion(tmpdir)
        test_empty_missing_and_differently_named_definitions(tmpdir)


if __name__ == "__main__":
    main()
