#!/usr/bin/env python3

import pathlib
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


ROOT = pathlib.Path(__file__).resolve().parents[2]
GENERATE_BSD = ROOT / "tools" / "generate_bsd.py"
GENERATE_DATATYPES = ROOT / "tools" / "generate_datatypes.py"
BSD_NS = {"opc": "http://opcfoundation.org/BinarySchema/"}

NODESET = """<?xml version="1.0"?>
<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
  <NamespaceUris><Uri>urn:open62541:test:definitions</Uri></NamespaceUris>
  <Models><Model ModelUri="urn:open62541:test:definitions"/></Models>
  <Aliases>
    <Alias Alias="HasSubtype">i=45</Alias>
    <Alias Alias="Enumeration">i=29</Alias>
    <Alias Alias="Structure">i=22</Alias>
    <Alias Alias="String">i=12</Alias>
    <Alias Alias="Byte">i=3</Alias>
    <Alias Alias="UInt32">i=7</Alias>
    <Alias Alias="OptionSet">i=12755</Alias>
    <Alias Alias="BaseStructure">ns=1;i=2</Alias>
    <Alias Alias="TestEnum">ns=1;i=1</Alias>
  </Aliases>
  <UADataType NodeId="ns=1;i=1" BrowseName="1:TestEnum">
    <References><Reference ReferenceType="HasSubtype" IsForward="false">Enumeration</Reference></References>
    <Definition Name="1:TestEnum">
      <Field Name="Invalid,Name" SymbolicName="ValidName" Value="0"/>
    </Definition>
  </UADataType>
  <UADataType NodeId="ns=1;i=2" BrowseName="1:BaseStructure">
    <References><Reference ReferenceType="HasSubtype" IsForward="false">Structure</Reference></References>
    <Definition Name="1:BaseStructure">
      <Field Name="BaseValue" DataType="String"/>
    </Definition>
  </UADataType>
  <UADataType NodeId="ns=1;i=3" BrowseName="1:DerivedStructure">
    <References><Reference ReferenceType="HasSubtype" IsForward="false">BaseStructure</Reference></References>
    <Definition Name="1:DerivedStructure">
      <Field Name="Values" DataType="UInt32" ValueRank="1"/>
      <Field Name="Label" DataType="String" IsOptional="true"/>
      <Field Name="AnyValue" DataType="Structure" AllowSubTypes="true"/>
    </Definition>
  </UADataType>
  <UADataType NodeId="ns=1;i=4" BrowseName="1:TestUnion">
    <References><Reference ReferenceType="HasSubtype" IsForward="false">Structure</Reference></References>
    <Definition Name="1:TestUnion" IsUnion="true">
      <Field Name="Number" DataType="UInt32"/>
      <Field Name="Text" DataType="String"/>
    </Definition>
  </UADataType>
  <UADataType NodeId="ns=1;i=5" BrowseName="1:ByteOptionSet">
    <References><Reference ReferenceType="HasSubtype" IsForward="false">Byte</Reference></References>
    <Definition Name="1:ByteOptionSet" IsOptionSet="true">
      <Field Name="Enabled" Value="0"/>
    </Definition>
  </UADataType>
  <UADataType NodeId="ns=1;i=6" BrowseName="1:StructureOptionSet">
    <References><Reference ReferenceType="HasSubtype" IsForward="false">OptionSet</Reference></References>
    <Definition Name="1:StructureOptionSet" IsOptionSet="true">
      <Field Name="Enabled" Value="0"/>
    </Definition>
  </UADataType>
</UANodeSet>
"""

CSV = """TestEnum,1,DataType
BaseStructure,2,DataType
DerivedStructure,3,DataType
TestUnion,4,DataType
ByteOptionSet,5,DataType
StructureOptionSet,6,DataType
TestEnum_Encoding_DefaultBinary,11,Object
BaseStructure_Encoding_DefaultBinary,12,Object
DerivedStructure_Encoding_DefaultBinary,13,Object
TestUnion_Encoding_DefaultBinary,14,Object
ByteOptionSet_Encoding_DefaultBinary,15,Object
StructureOptionSet_Encoding_DefaultBinary,16,Object
"""


def main():
    with tempfile.TemporaryDirectory() as directory:
        temp = pathlib.Path(directory)
        nodeset = temp / "definitions.xml"
        csv = temp / "definitions.csv"
        bsd = temp / "definitions.bsd"
        output = temp / "types_definitions"
        nodeset.write_text(NODESET, encoding="utf-8")
        csv.write_text(CSV, encoding="utf-8")

        subprocess.run([sys.executable, str(GENERATE_BSD), "--xml", str(nodeset),
                        str(bsd)], check=True)
        root = ET.parse(bsd).getroot()
        enums = root.findall("opc:EnumeratedType", BSD_NS)
        structures = root.findall("opc:StructuredType", BSD_NS)
        assert [item.get("Name") for item in enums] == ["TestEnum", "ByteOptionSet"]
        assert enums[0].find("opc:EnumeratedValue", BSD_NS).get("Name") == "ValidName"
        assert enums[1].get("LengthInBits") == "8"
        assert enums[1].get("IsOptionSet") == "true"

        derived = next(item for item in structures
                       if item.get("Name") == "DerivedStructure")
        fields = derived.findall("opc:Field", BSD_NS)
        field_names = [field.get("Name") for field in fields]
        assert field_names == ["LabelSpecified", "Reserved1", "BaseValue",
                               "NoOfValues", "Values", "Label", "AnyValue"]
        assert fields[-1].get("TypeName") == "ua:ExtensionObject"

        union = next(item for item in structures if item.get("Name") == "TestUnion")
        union_fields = union.findall("opc:Field", BSD_NS)
        assert union.get("BaseType") == "ua:Union"
        assert union_fields[1].get("SwitchField") == "SwitchField"
        assert union_fields[2].get("SwitchValue") == "2"

        structure_option_set = next(item for item in structures
                                    if item.get("Name") == "StructureOptionSet")
        option_fields = structure_option_set.findall("opc:Field", BSD_NS)
        assert [field.get("Name") for field in option_fields] == ["Value", "ValidBits"]

        subprocess.run([sys.executable, str(GENERATE_DATATYPES), "--no-builtin",
                        "--type-bsd=" + str(bsd), "--type-csv=" + str(csv),
                        "--xml=" + str(nodeset), str(output)], check=True)
        generated = output.with_name(output.name + "_generated.h").read_text()
        assert "UA_DerivedStructure" in generated
        assert "UA_TestUnion" in generated


if __name__ == "__main__":
    main()
