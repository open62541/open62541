#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
import csv
import ast
import base64
import re
from xml.dom import minidom
import xml.etree.ElementTree as ET
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from nodeset_compiler.nodeset import extractNamespaces, NodeSet
from nodeset_compiler.datatypes import NodeId, LocalizedText
from nodeset_compiler.backend_open62541 import makeCLiteral, makeCIdentifier, splitStringLiterals
from nodeset_compiler.type_parser import OpaqueType


def test_literals_and_opaque_metadata():
    value = ' leading ' + ('a\\b"c\n\r\\' * 140) + ' trailing '
    for length in (1, 2, 3, 5, 500):
        assert ast.literal_eval('(' + splitStringLiterals(makeCLiteral(value), length) + ')') == value
    for name in ('unsigned', 'default', 'class', 'and_eq', 'char16_t', '3DRecord'):
        assert makeCIdentifier(name) != name
    text = LocalizedText(minidom.parseString(
        '<LocalizedText Locale="en"> text with spaces </LocalizedText>').documentElement)
    assert text.locale == 'en' and text.text == ' text with spaces '
    xml = ET.fromstring('<OpaqueType Name="Alias"/>')
    assert OpaqueType.from_bsd('types_test', 'urn:test', 'UInt32', xml).pointerfree
    assert not OpaqueType.from_bsd('types_test', 'urn:test', 'String', xml).pointerfree


def run_nodeset(output, *arguments):
    return subprocess.run([sys.executable, str(ROOT/'tools/nodeset_compiler/nodeset_compiler.py'),
                           *map(str, arguments), str(output)], capture_output=True, text=True)


def test_reference_dependencies(tmp):
    # Non-hierarchical dictionary links may point outside the supplied closure;
    # missing structural dependencies must still be rejected.
    document = '''<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
      <NamespaceUris><Uri>urn:references</Uri></NamespaceUris>
      <UAReferenceType NodeId="ns=1;i=2" BrowseName="1:CustomOrganizes">
        <DisplayName>CustomOrganizes</DisplayName><References>
          <Reference ReferenceType="i=45" IsForward="false">i=35</Reference>
        </References></UAReferenceType>
      <UAObject NodeId="ns=1;i=1" BrowseName="1:Source"><DisplayName>Source</DisplayName>
        <References><Reference ReferenceType="i=40">i=61</Reference>
          <Reference ReferenceType="REF">ns=1;i=9999</Reference>
        </References></UAObject></UANodeSet>'''
    nodeset = tmp/'reference_dependencies.xml'
    standard = ROOT/'tools/schema/Opc.Ua.NodeSet2.Reduced.xml'
    nodeset.write_text(document.replace('REF', 'i=39'))
    result = run_nodeset(tmp/'external_generated', '--existing', standard, '--xml', nodeset)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'WARNING' in result.stdout and 'will not be generated' in result.stdout
    for reference_type, diagnostic in [('i=35', 'unknown target'),
                                       ('ns=1;i=2', 'unknown target'),
                                       ('ns=1;i=9998', 'unknown reference type')]:
        nodeset.write_text(document.replace('REF', reference_type))
        result = run_nodeset(tmp/'missing_dependency_generated', '--existing', standard, '--xml', nodeset)
        assert result.returncode != 0 and diagnostic in result.stdout + result.stderr, result.stdout + result.stderr


def test_embedded_standard_structures(tmp):
    dictionary = '''<opc:TypeDictionary xmlns:opc="http://opcfoundation.org/BinarySchema/"
        xmlns:ua="http://opcfoundation.org/UA/" TargetNamespace="urn:legacy">
        <opc:StructuredType Name="LegacyPosition">
          <opc:Field Name="Position" TypeName="ua:3DCartesianCoordinates"/>
        </opc:StructuredType></opc:TypeDictionary>'''
    encoded = base64.b64encode(dictionary.encode()).decode()
    dependency = tmp/'legacy_dictionary.xml'
    dependency.write_text(f'''<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
      <NamespaceUris><Uri>urn:legacy</Uri></NamespaceUris>
      <UAVariable NodeId="ns=1;i=1" BrowseName="1:Dictionary" SymbolicName="Legacy_BinarySchema" DataType="i=15">
        <DisplayName>Dictionary</DisplayName>
        <References><Reference ReferenceType="i=40">i=63</Reference></References>
        <Value><ByteString>{encoded}</ByteString></Value>
      </UAVariable></UANodeSet>''')
    root = tmp/'legacy_root.xml'
    root.write_text('''<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd">
      <NamespaceUris><Uri>urn:legacy-root</Uri></NamespaceUris>
      <UAObject NodeId="ns=1;i=2" BrowseName="1:Root"><DisplayName>Root</DisplayName>
        <References><Reference ReferenceType="i=40">i=61</Reference></References>
      </UAObject></UANodeSet>''')
    result = run_nodeset(tmp/'legacy_generated', '--existing',
        ROOT/'tools/schema/Opc.Ua.NodeSet2.Reduced.xml', '--existing', dependency, '--xml', root)
    assert result.returncode == 0, result.stdout + result.stderr
from nodeset_compiler.backend_open62541 import splitStringLiterals
from nodeset_compiler.backend_open62541_datatypes import CGenerator, makeCIdentifier, makeCLiteral
from nodeset_compiler.type_parser import EnumerationType


def test_c_literals_and_identifiers():
    # A literal boundary may land within a backslash escape or at whitespace.
    value = ' leading whitespace ' + ('a\\b"c\n\r\\' * 140) + ' trailing whitespace '
    for length in (1, 2, 3, 5, 500):
        literal = splitStringLiterals(makeCLiteral(value), length)
        assert ast.literal_eval('(' + literal + ')') == value
    for name in ('unsigned', 'default', '3DCoordinate', 'class'):
        identifier = makeCIdentifier(name)
        assert identifier != name and re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', identifier)
    enum = EnumerationType('types_test', 'urn:test', 'Labels')
    enum.elements.update({'A+B': '0', 'A-B': '1', 'A B': '2', 'A+B_1': '3'})
    declaration = CGenerator.print_enum_typedef(enum)
    labels = re.findall(r'^    (UA_\w+) =', declaration, re.MULTILINE)
    assert len(labels) == len(set(labels)) == 4
    text = LocalizedText(minidom.parseString(
        '<LocalizedText Locale="en"> text with spaces </LocalizedText>').documentElement)
    assert text.text == ' text with spaces ' and text.locale == 'en'



def main():
    test_literals_and_opaque_metadata()
    test_c_literals_and_identifiers()
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        test_reference_dependencies(tmp)
        test_embedded_standard_structures(tmp)
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
