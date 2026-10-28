#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from nodeset_compiler.nodeset import extractNamespaces, NodeSet
from nodeset_compiler.datatypes import NodeId


def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        uri = tmp / 'uris.xml'
        uri.write_text('<UANodeSet><NamespaceUris><Uri>urn:a</Uri><Uri>urn:a</Uri>'
                       '<Uri>urn:b</Uri></NamespaceUris><Other/></UANodeSet>')
        with uri.open() as f:
            uris = extractNamespaces(f)
        assert uris == ['http://opcfoundation.org/UA/', 'urn:a', 'urn:a', 'urn:b']
        nodes = NodeSet()
        for value in uris:
            nodes.addNamespace(value)
        nodes.createNamespaceMapping(uris)
        assert nodes.namespaceMapping == {0: 0, 1: 1, 2: 1, 3: 2}
        uri.write_text('<root xmlns:p="urn:x"><p:NamespaceUris><p:Uri>urn:a</p:Uri>'
                       '</p:NamespaceUris></root>')
        with uri.open() as f:
            assert extractNamespaces(f)[1:] == ['urn:a']
        for identifier in ['ns=2;s=semi;slash\\quote"',
                           'ns=2;g=00000001-0002-0003-0004-000000000005']:
            value = NodeId(identifier)
            assert str(value).lower() == identifier.lower()
        bsd = tmp / 'types.bsd'
        bsd.write_text('<opc:TypeDictionary xmlns:opc="http://opcfoundation.org/BinarySchema/" '
                       'TargetNamespace="urn:test"><opc:StructuredType Name="Point">'
                       '<opc:Field Name="Value" TypeName="opc:UInt32"/>'
                       '</opc:StructuredType></opc:TypeDictionary>')
        ids = tmp / 'ids.csv'
        with ids.open('w', newline='') as f:
            csv.writer(f).writerow(['Point', 'ns=2;s=semi;slash\\quote"', 'DataType'])
        # POSIX must also derive the correct C name from a Windows output path.
        output = tmp / 'windows\\types_portable'
        output.parent.mkdir(parents=True, exist_ok=True)
        result = subprocess.run([sys.executable, str(ROOT/'tools/generate_datatypes.py'),
                                 '--type-bsd', str(bsd), '--type-csv', str(ids),
                                 '--no-builtin', str(output)], capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
        header = Path(str(output)+'_generated.h').read_text()
        source = Path(str(output)+'_generated.c').read_text()
        assert '#define UA_TYPES_PORTABLE_COUNT 1' in header
        assert 'UA_STRING_STATIC("semi;slash\\\\quote\\\"")' in source, source


if __name__ == '__main__':
    main()
