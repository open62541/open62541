#!/usr/bin/env python3

import os
import sys
from contextlib import ExitStack
from pathlib import Path


HERE = Path(__file__).resolve().parent
SOURCE_DIR = HERE.parent.parent
NODESET_DIR = SOURCE_DIR / "deps" / "ua-nodeset"

sys.path.insert(0, os.fspath(SOURCE_DIR / "tools"))
from nodeset_compiler.datatypes import NodeId
from nodeset_compiler.nodes import DataTypeNode
from nodeset_compiler.nodeset import NodeSet
from nodeset_compiler.type_parser import (CSVBSDTypeParser, EnumerationType,
                                          OpaqueType, StructType, TypeParser,
                                          get_definition_fields)


HAS_SUBTYPE = NodeId("ns=0;i=45")


CASES = (
    {
        "name": "DI",
        "xml": NODESET_DIR / "DI" / "Opc.Ua.Di.NodeSet2.xml",
        "bsd": NODESET_DIR / "DI" / "Opc.Ua.Di.Types.bsd",
        "csv": NODESET_DIR / "DI" / "Opc.Ua.Di.NodeIds.csv",
        "dependencies": (),
        "imports": (),
    },
    {
        "name": "ADI",
        "xml": NODESET_DIR / "ADI" / "Opc.Ua.Adi.NodeSet2.xml",
        "bsd": NODESET_DIR / "ADI" / "Opc.Ua.Adi.Types.bsd",
        "csv": NODESET_DIR / "ADI" / "OpcUaAdiModel.csv",
        "dependencies": (NODESET_DIR / "DI" / "Opc.Ua.Di.NodeSet2.xml",),
        "imports": (("TYPES_DI",
                     NODESET_DIR / "DI" / "Opc.Ua.Di.Types.bsd"),),
    },
    {
        "name": "AutoID",
        "xml": NODESET_DIR / "AutoID" / "Opc.Ua.AutoID.NodeSet2.xml",
        "bsd": HERE / "Opc.Ua.AutoID.Types.bsd",
        "csv": HERE / "Opc.Ua.AutoID.NodeIds.csv",
        "dependencies": (NODESET_DIR / "DI" / "Opc.Ua.Di.NodeSet2.xml",),
        "imports": (("TYPES_DI",
                     NODESET_DIR / "DI" / "Opc.Ua.Di.Types.bsd"),),
    },
    {
        "name": "testnodeset",
        "xml": HERE / "testnodeset.xml",
        "bsd": HERE / "testtypes.bsd",
        "csv": HERE / "testnodeset.csv",
        "dependencies": (NODESET_DIR / "DI" / "Opc.Ua.Di.NodeSet2.xml",),
        "imports": (("TYPES_DI",
                     NODESET_DIR / "DI" / "Opc.Ua.Di.Types.bsd"),),
    },
)


def definition_fields(node):
    """Return layout fields declared by a non-namespace-zero datatype."""
    if node.id.ns == 0 or node.dataTypeDefinition is None:
        return []
    return get_definition_fields(node.dataTypeDefinition)


def parse_inline(case):
    """Normalize the case's inline NodeSet definitions by datatype name."""
    nodeset = NodeSet()
    with ExitStack() as stack:
        stream = stack.enter_context(
            (NODESET_DIR / "Schema" / "Opc.Ua.NodeSet2.xml").open("rb"))
        nodeset.addNodeSet(stream, hidden=True, typesArray="UA_TYPES")
        for path in case["dependencies"]:
            stream = stack.enter_context(path.open("rb"))
            nodeset.addNodeSet(stream, hidden=True, typesArray="UA_TYPES")
        stream = stack.enter_context(case["xml"].open("rb"))
        nodeset.addNodeSet(stream, typesArray="UA_TYPES")

    parser = TypeParser({}, [], "inline", nodeset.namespaceMapping)
    definitions = []
    aliases = []
    for node in nodeset.nodes.values():
        if not isinstance(node, DataTypeNode):
            continue
        fields = definition_fields(node)
        if fields:
            base_id = None
            for ref in node.references:
                if ref.referenceType == HAS_SUBTYPE and not ref.isForward:
                    base_id = ref.target
                    break
            definitions.append((node.dataTypeDefinition,
                                nodeset.namespaces[node.id.ns], node.id,
                                not node.hidden, base_id))
        else:
            for ref in node.references:
                if ref.referenceType == HAS_SUBTYPE and not ref.isForward:
                    aliases.append((node.id, ref.target))
                    break
    parser.addTypesFromDefinitions(definitions, aliases)
    return {datatype.name: datatype
            for namespace in parser.types.values()
            for datatype in namespace.values() if datatype.isLocal}


def parse_bsd(case):
    """Normalize the case's legacy BinarySchema definitions by name."""
    outname = "bsd_compare"
    imports = [f"{name}#{path}" for name, path in case["imports"]]
    with ExitStack() as stack:
        bsd = stack.enter_context(case["bsd"].open(encoding="utf-8"))
        csv = stack.enter_context(case["csv"].open(encoding="utf-8"))
        xml = stack.enter_context(case["xml"].open("rb"))
        parser = CSVBSDTypeParser(
            [], [], True, outname, imports, [bsd], [csv], [xml],
            {"http://opcfoundation.org/UA/": 0})
        parser.create_types()
    return {datatype.name: datatype
            for namespace in parser.types.values()
            for datatype in namespace.values()
            if (datatype.outname == outname and
                not (isinstance(datatype, StructType) and
                     not datatype.members))}


def type_signature(datatype):
    """Reduce one datatype to the layout properties shared by both formats."""
    if isinstance(datatype, EnumerationType):
        elements = datatype.elements.items()
        if datatype.isOptionSet:
            elements = ((name, value) for name, value in elements
                        if int(value) != 0)
        return ("option" if datatype.isOptionSet else "enum",
                datatype.strDataType,
                tuple(elements))
    if isinstance(datatype, OpaqueType):
        return ("opaque", datatype.base_type)
    if isinstance(datatype, StructType):
        members = tuple((member.name, member.member_type.name,
                         bool(member.is_array), bool(member.is_optional))
                        for member in datatype.members)
        return ("union" if datatype.is_union else "struct", members)
    return (type(datatype).__name__,)


def compare_case(case):
    """Report missing or structurally different types for one model."""
    inline = parse_inline(case)
    bsd = parse_bsd(case)
    errors = []
    inline_names = set(inline)
    bsd_names = set(bsd)
    for name in sorted(inline_names - bsd_names):
        errors.append(f"{case['name']}: only inline XML defines {name}")
    for name in sorted(bsd_names - inline_names):
        errors.append(f"{case['name']}: only BSD defines {name}")
    for name in sorted(inline_names & bsd_names):
        inline_signature = type_signature(inline[name])
        bsd_signature = type_signature(bsd[name])
        if inline_signature != bsd_signature:
            errors.append(
                f"{case['name']}: {name} differs\n"
                f"  inline: {inline_signature}\n"
                f"  BSD:    {bsd_signature}")
    return errors


def main():
    """Compare every representative model and print focused differences."""
    errors = []
    for case in CASES:
        try:
            case_errors = compare_case(case)
        except Exception as error:
            case_errors = [f"{case['name']}: comparison failed: {error}"]
        errors.extend(case_errors)
        if not case_errors:
            print(f"PASS: {case['name']} inline and BSD definitions agree")
    if errors:
        print("\n".join(errors))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
