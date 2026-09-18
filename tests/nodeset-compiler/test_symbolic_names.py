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
  <opc:StructuredType Name="ThreeDFrame" BaseType="ua:ExtensionObject">
    <opc:Field Name="Value" TypeName="opc:UInt32" />
  </opc:StructuredType>
</opc:TypeDictionary>
"""

NODESET = """<?xml version="1.0" encoding="utf-8"?>
<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
  <NamespaceUris>
    <Uri>urn:open62541:test</Uri>
  </NamespaceUris>
  <UADataType NodeId="ns=1;i=1" BrowseName="1:3DFrame"
              SymbolicName="ThreeDFrame">
    <Definition Name="3DFrame" SymbolicName="ThreeDFrame">
      <Field Name="Value" DataType="i=7" />
    </Definition>
  </UADataType>
</UANodeSet>
"""

CSV = """ThreeDFrame,1,DataType
ThreeDFrame_Encoding_DefaultBinary,2,Object
"""


def write(path, content):
    with open(path, "w", encoding="utf-8") as output:
        output.write(content)


def main():
    with tempfile.TemporaryDirectory() as tmpdir:
        bsd = os.path.join(tmpdir, "types.bsd")
        nodeset = os.path.join(tmpdir, "nodeset.xml")
        nodeids = os.path.join(tmpdir, "nodeids.csv")
        output = os.path.join(tmpdir, "types_test")
        write(bsd, BSD)
        write(nodeset, NODESET)
        write(nodeids, CSV)

        command = [
            sys.executable,
            GENERATOR,
            f"--type-bsd={bsd}",
            f"--type-csv={nodeids}",
            f"--xml={nodeset}",
            "--no-builtin",
            output,
        ]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0:
            print(result.stderr)
            return 1

        with open(output + "_generated.c", encoding="utf-8") as generated:
            source = generated.read()

        type_id = "{0, UA_NODEIDTYPE_NUMERIC, {1LU}}, /* .typeId */"
        if type_id not in source:
            print("FAIL: generated output lacks the ThreeDFrame NodeId")
            return 1
        return 0


if __name__ == "__main__":
    sys.exit(main())
