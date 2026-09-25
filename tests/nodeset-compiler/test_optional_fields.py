#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import subprocess
import sys
import tempfile


HERE = os.path.dirname(os.path.abspath(__file__))
GENERATOR = os.path.join(HERE, "..", "..", "tools", "generate_datatypes.py")

BSD = """<?xml version="1.0" encoding="utf-8"?>
<opc:TypeDictionary
  xmlns:opc="http://opcfoundation.org/BinarySchema/"
  xmlns:ua="http://opcfoundation.org/UA/"
  xmlns:tns="urn:open62541:test"
  TargetNamespace="urn:open62541:test">
  <opc:StructuredType Name="OptionalStructure" BaseType="ua:ExtensionObject">
    <opc:Field Name="Required" TypeName="opc:UInt32" />
    <opc:Field Name="Optional" TypeName="opc:String" />
  </opc:StructuredType>
</opc:TypeDictionary>
"""

NODESET = """<?xml version="1.0" encoding="utf-8"?>
<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
  <NamespaceUris>
    <Uri>urn:open62541:test</Uri>
  </NamespaceUris>
  <UADataType NodeId="ns=1;i=1" BrowseName="1:OptionalStructure">
    <Definition Name="OptionalStructure">
      <Field Name="Required" DataType="i=7" />
      <Field Name="Optional" DataType="i=12" IsOptional="true" />
    </Definition>
  </UADataType>
</UANodeSet>
"""

CSV = """OptionalStructure,1,DataType
OptionalStructure_Encoding_DefaultBinary,2,Object
"""


def write(path, content):
    with open(path, "w", encoding="utf-8") as output:
        output.write(content)


def main():
    with tempfile.TemporaryDirectory() as tmpdir:
        bsd = os.path.join(tmpdir, "types.bsd")
        nodeset = os.path.join(tmpdir, "nodeset.xml")
        csv = os.path.join(tmpdir, "nodeids.csv")
        output = os.path.join(tmpdir, "types_test")
        write(bsd, BSD)
        write(nodeset, NODESET)
        write(csv, CSV)

        command = [
            sys.executable,
            GENERATOR,
            f"--type-bsd={bsd}",
            f"--type-csv={csv}",
            f"--xml={nodeset}",
            "--no-builtin",
            output,
        ]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            print(result.stderr)
            return 1

        with open(output + "_generated.h", encoding="utf-8") as generated:
            header = generated.read()
        with open(output + "_generated.c", encoding="utf-8") as generated:
            source = generated.read()

        expected = {
            "optional member pointer": "UA_String *optional;" in header,
            "optional structure kind": "UA_DATATYPEKIND_OPTSTRUCT" in source,
            "optional member metadata": "true  /* .isOptional */" in source,
        }
        failures = [name for name, present in expected.items() if not present]
        for failure in failures:
            print(f"FAIL: generated output lacks {failure}")
        return len(failures)


if __name__ == "__main__":
    sys.exit(main())
