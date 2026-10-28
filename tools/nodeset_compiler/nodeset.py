#!/usr/bin/env python3

### This Source Code Form is subject to the terms of the Mozilla Public
### License, v. 2.0. If a copy of the MPL was not distributed with this
### file, You can obtain one at http://mozilla.org/MPL/2.0/.

###    Copyright 2014-2015 (c) TU-Dresden (Author: Chris Iatrou)
###    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
###    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH

import xml.dom.minidom as dom
import xml.etree.ElementTree as ET
import logging
import codecs
import re
from pathlib import Path
import copy
from collections import OrderedDict

from .datatypes import NodeId
from .nodes import *
from .opaque_type_mapping import opaque_type_mapping
from .type_parser import (CSVBSDTypeParser, TypeParser, StructType,
                          get_definition_fields)

import io
import tempfile
import base64

__all__ = ['NodeSet', 'getSubTypesOf']

logger = logging.getLogger(__name__)

####################
# Helper Functions #
####################

hassubtype = NodeId("ns=0;i=45")

def getSubTypesOf(nodeset, node, skipNodes=None):
    if skipNodes is None:
        skipNodes = []
    if node in skipNodes:
        return []
    re = set()
    re.add(node)
    for ref in node.references:
        if ref.referenceType == hassubtype:
            skipAll = set()
            skipAll.update(skipNodes)
            skipAll.update(re)
            if (ref.source == node.id and ref.isForward):
                re.update(getSubTypesOf(nodeset, nodeset.nodes[ref.target], skipNodes=skipAll))
            elif (ref.target == node.id and not ref.isForward):
                re.update(getSubTypesOf(nodeset, nodeset.nodes[ref.source], skipNodes=skipAll))
    return re

def _definitionFields(node):
    if node.id.ns == 0 or node.dataTypeDefinition is None:
        return []
    return get_definition_fields(node.dataTypeDefinition)

def _baseDataTypeId(node):
    for ref in node.references:
        if (ref.referenceType == NodeId("ns=0;i=45") and
                not ref.isForward):
            return ref.target
    return None

def extractNamespaces(xmlfile):
    """Read the URI table independently of XML formatting and prefixes."""
    namespaces = ["http://opcfoundation.org/UA/"]
    with open(xmlfile.name, "rb") as source:
        for _, element in ET.iterparse(source, events=("end",)):
            if element.tag.rsplit("}", 1)[-1] != "NamespaceUris":
                continue
            for uri in element:
                namespaces.append(uri.text or "")
            break
    return namespaces

def buildAliasList(xmlelement):
    """Parses the <Alias> XML Element present in must XML NodeSet definitions.
       Contents the Alias element are stored in a dictionary for further
       dereferencing during pointer linkage (see linkOpenPointer())."""
    aliases = {}
    for al in xmlelement.childNodes:
        if al.nodeType == al.ELEMENT_NODE:
            if al.hasAttribute("Alias"):
                aliasst = al.getAttribute("Alias")
                aliasnd = al.firstChild.data
                aliases[aliasst] = aliasnd
    return aliases

class NodeSet:
    """ This class handles parsing XML description of namespaces, instantiating
        nodes, linking references, graphing the namespace and compiling a binary
        representation.

        Note that nodes assigned to this class are not restricted to having a
        single namespace ID. This class represents the entire physical address
        space of the binary representation and all nodes that are to be included
        in that segment of memory.
    """

    def __init__(self):
        self.nodes = OrderedDict()
        self.aliases = {}
        self.namespaces = ["http://opcfoundation.org/UA/"]
        self.namespaceMapping = {}

    def sanitize(self):
        # Hierarchical/type dependencies must exist. Other references may
        # legitimately point outside the supplied model closure.
        required = set(self.getRelevantOrderingReferences())
        for n in self.nodes.values():
            for ref in n.references:
                if not ref.source == n.id:
                    raise Exception("Reference " + str(ref) + " has an invalid source")
                if ref.referenceType not in self.nodes:
                    raise Exception("Reference " + str(ref) + " has an unknown reference type")
                if ref.target not in self.nodes:
                    if ref.referenceType in required:
                        raise Exception("Reference " + str(ref) + " has an unknown target")
                    logger.warning("Reference %s points outside the supplied NodeSets; "
                                   "it will not be generated", ref)

    def addNamespace(self, nsURL):
        if nsURL not in self.namespaces:
            self.namespaces.append(nsURL)

    def createNamespaceMapping(self, orig_namespaces):
        """Creates a dict that maps from the nsindex in the original nodeset to the
           nsindex in the combined nodeset"""
        self.namespaceMapping = {}
        for index, name in enumerate(orig_namespaces):
            self.namespaceMapping[index] = self.namespaces.index(name)

    def getNodeByBrowseName(self, idstring):
        return next((n for n in self.nodes.values() if idstring == n.browseName.name), None)

    def getRoot(self):
        return self.getNodeByBrowseName("Root")

    def createNode(self, xmlelement, modelUri, hidden=False):
        ndtype = xmlelement.localName.lower()
        if ndtype[:2] == "ua":
            ndtype = ndtype[2:]

        node = None
        if ndtype == 'variable':
            node = VariableNode(xmlelement)
        if ndtype == 'object':
            node = ObjectNode(xmlelement)
        if ndtype == 'method':
            node = MethodNode(xmlelement)
        if ndtype == 'objecttype':
            node = ObjectTypeNode(xmlelement)
        if ndtype == 'variabletype':
            node = VariableTypeNode(xmlelement)
        if ndtype == 'methodtype':
            node = MethodNode(xmlelement)
        if ndtype == 'datatype':
            node = DataTypeNode(xmlelement)
        if ndtype == 'referencetype':
            node = ReferenceTypeNode(xmlelement)
        if ndtype == 'view':
            node = ViewNode(xmlelement)

        if node is None:
            return None

        node.modelUri = modelUri
        node.hidden = hidden
        return node

    def hide_node(self, nodeId, hidden=True):
        if nodeId not in self.nodes:
            return False
        node = self.nodes[nodeId]
        node.hidden = hidden
        return True

    def merge_dicts(self, *dict_args):
        """
        Given any number of dicts, shallow copy and merge into a new dict,
        precedence goes to key value pairs in latter dicts.
        """
        result = {}
        for dictionary in dict_args:
            result.update(dictionary)
        return result

    def getNodeByIDString(self, idStr):
        # Split id to namespace part and id part
        m = re.match("ns=([^;]+);(.*)", idStr)
        if m:
            ns = m.group(1)
            # Convert namespace uri to index
            if not ns.isdigit():
                if ns not in self.namespaces:
                    return None
                ns = self.namespaces.index(ns)
                idStr = f"ns={ns};{m.group(2)}"
        nodeId = NodeId(idStr)
        if nodeId not in self.nodes:
            return None
        return self.nodes[nodeId]

    def remove_node(self, node):

        def filterRef(r, rt):
            return (r.referenceType != rt.referenceType) or (not (
                    rt.target == node.id or rt.source == node.id
                ))

        for r in node.references:
            if r.target == node.id:
                if r.source not in self.nodes:
                    continue
                self.nodes[r.source].references = dict(filter(
                    lambda rt: filterRef(r, rt[0]), # filter only on key of each dict item
                    self.nodes[r.source].references.items()
                ))
            elif r.source == node.id:
                if r.target not in self.nodes:
                    continue
                self.nodes[r.target].references = dict(filter(
                    lambda rt: filterRef(r, rt[0]), # filter only on key of each dict item
                    self.nodes[r.target].references.items()
                ))
        del self.nodes[node.id]


    def addNodeSet(self, xmlfile, hidden=False, typesArray="UA_TYPES"):
        # Extract NodeSet DOM

        fileContent = xmlfile.read()
        # Remove BOM since the dom parser cannot handle it on Python 3 Windows
        if fileContent.startswith( codecs.BOM_UTF8 ):
            fileContent = fileContent.lstrip( codecs.BOM_UTF8 )
        fileContent = fileContent.decode("utf-8")

        # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
        fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\\g<1>\\g<2>\\g<3>>", fileContent)

        nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
        if len(nodesets) == 0 or len(nodesets) > 1:
            raise Exception(self, self.originXML + " contains no or more then 1 nodeset")
        nodeset = nodesets[0]

        # Extract the modelUri
        try:
            modelTag = nodeset.getElementsByTagName("Models")[0].getElementsByTagName("Model")[0]
            modelUri = modelTag.attributes["ModelUri"].nodeValue
        except Exception:
            # Ignore exception and try to use namespace array
            modelUri = None

        # Create the namespace mapping
        orig_namespaces = extractNamespaces(xmlfile)  # List of namespaces used in the xml file
        if modelUri is None and len(orig_namespaces) > 1:
            modelUri = orig_namespaces[1]

        if modelUri is None:
            raise Exception(self, self.originXML + " does not define the nodeset URI in Models/Model/ModelUri or NamespaceUris array.")

        for ns in orig_namespaces:
            self.addNamespace(ns)
        self.createNamespaceMapping(orig_namespaces) # mapping for this file

        # Extract the aliases
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            ndtype = nd.localName.lower()
            if 'aliases' in ndtype:
                self.aliases = self.merge_dicts(self.aliases, buildAliasList(nd))

        # Instantiate nodes
        newnodes = {}
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            node = self.createNode(nd, modelUri, hidden)
            if not node:
                continue
            node.replaceAliases(self.aliases)
            node.replaceNamespaces(self.namespaceMapping)
            node.typesArray = typesArray

            # Add the node the the global dict
            if node.id in self.nodes:
                raise Exception("XMLElement with duplicate ID " + str(node.id))
            self.nodes[node.id] = node
            newnodes[node.id] = node

    def getBaseDataType(self, node):
        if node is None:
            return None
        if node.browseName.name not in opaque_type_mapping:
            return node
        for ref in node.references:
            if ref.isForward:
                continue
            if ref.referenceType.i == 45:
                return self.getBaseDataType(self.nodes[ref.target])
        return node

    def getNodeTypeDefinition(self, node):
        for ref in node.references:
            # 40 = HasTypeDefinition
            if ref.referenceType.i == 40 and ref.isForward:
                return self.nodes[ref.target]
        return None

    def getDataTypeNode(self, dataType):
        if isinstance(dataType, str):
            if dataType in self.aliases:
                return self.getDataTypeNode(NodeId(self.aliases[dataType]))
            name = {"Variant": "BaseDataType", "ExtensionObject": "Structure"}.get(dataType, dataType)
            for node in self.nodes.values():
                if (isinstance(node, DataTypeNode) and node.id.ns == 0 and
                        1 <= node.id.i <= 25 and node.browseName.name == name):
                    return node
            logger.warning("Ignoring undeclared datatype alias %s; using the VariableType representation", dataType)
            return None
        if isinstance(dataType, NodeId):
            if dataType.i == 0:
                return None
            dataTypeNode = self.nodes[dataType]
            if not isinstance(dataTypeNode, DataTypeNode):
                logger.error("Node id %s is not reference a valid dataType.", dataType)
                return None
            return dataTypeNode
        return None

    def getRelevantOrderingReferences(self):
        # sanitize() runs before inverse references are synthesized. Derive the
        # reference hierarchy from either direction, including custom subtypes.
        required = {NodeId("i=33"), NodeId("i=38"), NodeId("i=40")}
        types = {node.id: node for node in self.nodes.values()
                 if isinstance(node, ReferenceTypeNode)}
        changed = True
        while changed:
            changed = False
            for node in types.values():
                for ref in node.references:
                    if ref.referenceType != hassubtype:
                        continue
                    parent, child = ((node.id, ref.target) if ref.isForward
                                     else (ref.target, node.id))
                    if parent in required and child in types and child not in required:
                        required.add(child)
                        changed = True
        return list(required)

    def addInverseReferences(self):
        # Ensure that every reference has an inverse reference in the target
        for u in self.nodes.values():
            for ref in u.references.copy():
                back = Reference(ref.target, ref.referenceType, ref.source, not ref.isForward)
                if ref.target not in self.nodes:
                    continue
                self.nodes[ref.target].references[back] = None # dict key does not make a duplicate entry

    def setNodeParent(self):
        parentreftypes = getSubTypesOf(self, self.getNodeByBrowseName("HierarchicalReferences"))
        parentreftypes = list(map(lambda x: x.id, parentreftypes))

        for node in self.nodes.values():
            if node.id.ns == 0 and node.id.i in [78, 80, 84]:
                # ModellingRule, Root node do not have a parent
                continue

            parentref = node.getParentReference(parentreftypes)
            if parentref is not None:
                node.parent = self.nodes[parentref.target]
                if not node.parent:
                    raise RuntimeError(f"Node {str(node.id)}: Did not find parent node: ")
                node.parentReference = self.nodes[parentref.referenceType]
            # Some nodes in the full nodeset do not have a parent. So accept this and do not show an error.
            #else:
            #    raise RuntimeError("Node {}: HierarchicalReference (or subtype of it) to parent node is missing.".format(str(node.id)))

    def normalizeDataTypes(self, typesOutname, generateTypes=False, typesArray=()):
        """Adapt NodeSet declarations to the existing datatype-generator model.

        NodeSet definitions declare local fields. Expand inheritance exactly
        once here; standard StructureDescriptions already carry all fields.
        """
        # Normalize inline definitions and externally owned BSD layouts.
        logger.info("Creating DataType definitions")
        typeParser = TypeParser({}, [], typesOutname, self.namespaceMapping)
        # UA_TYPES contains standard structures and enums as well as the 25
        # builtin wire types. Use the same schema as the library to resolve
        # fields such as EUInformation without regenerating standard types.
        schema = (Path(self.standardTypesSchema) if getattr(self, "standardTypesSchema", None)
                  else Path(__file__).resolve().parent.parent / "schema")
        with (schema / "NodeIds.csv").open() as nodeIds:
            standard = CSVBSDTypeParser(
                {}, [], False, "types", [], [str(schema / "Opc.Ua.Types.bsd")],
                [nodeIds], [], {0: "http://opcfoundation.org/UA/"})
            standard.create_types()
        for namespace, datatypes in standard.types.items():
            for datatype in datatypes.values():
                if datatype.nodeId is None:
                    continue
                identifier = str(datatype.nodeId)
                if "=" not in identifier:
                    identifier = "i=" + identifier
                datatype.nodeId = NodeId(identifier)
                typeParser.types.setdefault(namespace, OrderedDict())[datatype.name] = datatype
                typeParser.types_by_id[str(datatype.nodeId)] = datatype
        definitions = []
        aliases = []
        for node in self.nodes.values():
            if not isinstance(node, DataTypeNode):
                continue

            fields = _definitionFields(node)
            isLocal = False
            if fields:
                # Static generation owns every visible definition. The positional
                # types-array assignment is also used to validate namespace-only
                # generation without redeclaring BSD-backed dependency types.
                isLocal = not node.hidden and (generateTypes or
                                              node.typesArray == "UA_TYPES")
                definitionOutname = typesOutname
                if not isLocal and node.typesArray.startswith("UA_"):
                    definitionOutname = node.typesArray[3:].lower()
                definitions.append((node.dataTypeDefinition,
                                    self.namespaces[node.id.ns], node.id,
                                    isLocal, _baseDataTypeId(node),
                                    definitionOutname))

            # Prefer the representation already parsed from a supplied BSD. This
            # keeps NodeSet definitions usable for runtime metadata without
            # redeclaring datatypes owned by an external generated types array.
            if not isLocal and node.typesArray != "UA_TYPES":
                namespace_types = self.parser.types.get(
                    self.namespaces[node.id.ns], {})
                external = namespace_types.get(node.browseName.name)
                if external is not None:
                    external = copy.copy(external)
                    external.members = [copy.copy(member)
                                        for member in external.members]
                    external.nodeId = node.id
                    external.isLocal = False
                    if node.typesArray.startswith("UA_"):
                        external.outname = node.typesArray[3:].lower()
                    typeParser.types_by_id[str(node.id)] = external

            if not fields:
                baseTypeId = _baseDataTypeId(node)
                if baseTypeId is not None:
                    outname = (node.typesArray[3:].lower()
                               if node.typesArray.startswith("UA_") else typesOutname)
                    aliases.append((node.id, baseTypeId, node.browseName.name,
                                    self.namespaces[node.id.ns],
                                    not node.hidden and generateTypes,
                                    typesOutname if not node.hidden and generateTypes else outname))

        # Embedded and explicit BSD dictionaries are parsed before their NodeSet
        # namespace indexes and datatype-array owners are known. Rebind copied
        # structure members to the normalized NodeSet types so inherited layouts
        # do not retain stale UA_TYPES references.
        normalizedTypes = {
            (datatype.namespaceUri, datatype.name): datatype
            for datatype in typeParser.types_by_id.values()
        }
        for datatype in set(typeParser.types_by_id.values()):
            if not isinstance(datatype, StructType) or datatype.isLocal:
                continue
            for member in datatype.members:
                normalized = typeParser.types_by_id.get(str(member.data_type_id))
                if normalized is None:
                    normalized = normalizedTypes.get(
                        (member.member_type.namespaceUri, member.member_type.name))
                if normalized is not None:
                    member.member_type = normalized
        typeParser.addTypesFromDefinitions(definitions, aliases)

        # Encoding objects are graph nodes, not fields of Definition. Resolve
        # both reference directions after inverse references have been added.
        for datatype in set(typeParser.types_by_id.values()):
            node = self.nodes.get(datatype.nodeId)
            if node is None:
                continue
            for ref in node.references:
                if not ref.isForward or ref.referenceType != NodeId("i=38"):
                    continue
                encoding = self.nodes.get(ref.target)
                if encoding is None:
                    continue
                if encoding.browseName.name == "Default Binary":
                    datatype.binaryEncodingId = encoding.id
                elif encoding.browseName.name == "Default XML":
                    datatype.xmlEncodingId = encoding.id
        typeParser.namespaceUris = list(self.namespaces)

        localTypes = []
        visiting = set()
        visited = set()

        def addLocalType(datatype):
            """Append one local type after all local layout dependencies."""
            typeKey = id(datatype)
            if typeKey in visited:
                return
            if typeKey in visiting:
                raise RuntimeError(
                    "DataTypes involving %s have a cycle of inline members" %
                    datatype.name)
            visiting.add(typeKey)
            if isinstance(datatype, StructType):
                base = typeParser.get_type_for_id(datatype.baseTypeId)
                if base is not None and base.isLocal:
                    addLocalType(base)
                for member in datatype.members:
                    dependency = member.member_type
                    if not dependency.isLocal:
                        continue
                    # Pointer members only need a forward declaration. Inline
                    # members and enum/alias typedefs need a complete layout.
                    if ((member.is_array or member.is_optional) and
                            isinstance(dependency, StructType)):
                        if dependency is not datatype:
                            dependency.needs_forward_declaration = True
                        continue
                    addLocalType(dependency)
            visiting.remove(typeKey)
            visited.add(typeKey)
            localTypes.append(datatype)

        for datatypes in typeParser.types.values():
            for datatype in datatypes.values():
                if datatype.isLocal:
                    addLocalType(datatype)

        logger.info("Reordering nodes for minimal dependencies during printing")
        self.sortNodes()
        # Keep DataType nodes in declaration order before variables need them
        # for value type-checking. The namespace loader registers static layouts.
        orderedNodes = OrderedDict()
        for nodeId, node in self.nodes.items():
            if node.hidden:
                orderedNodes[nodeId] = node
        orderingDataTypes = set()

        def addDataTypeNode(nodeId):
            """Order a datatype node after the nodes needed by its layout."""
            node = self.nodes.get(nodeId)
            if node is None or node.hidden or node.id in orderedNodes:
                return
            if node.id in orderingDataTypes:
                raise RuntimeError(
                    "DataType inheritance cycle involving %s" % node.id)
            orderingDataTypes.add(node.id)
            baseTypeId = _baseDataTypeId(node)
            if baseTypeId is not None:
                addDataTypeNode(baseTypeId)
            orderingDataTypes.remove(node.id)
            orderedNodes[node.id] = node

        for datatype in localTypes:
            if isinstance(datatype, StructType):
                for member in datatype.own_members:
                    addDataTypeNode(member.data_type_id)
            addDataTypeNode(datatype.nodeId)
        for nodeId, node in self.nodes.items():
            if nodeId not in orderedNodes:
                orderedNodes[nodeId] = node
        self.nodes = orderedNodes
        generatedTypes = OrderedDict()
        importedTypes = OrderedDict()
        for datatype in typeParser.types_by_id.values():
            if datatype.isLocal:
                continue
            namespaceTypes = importedTypes.setdefault(datatype.namespaceUri,
                                                       OrderedDict())
            namespaceTypes[datatype.name] = datatype
        for datatype in localTypes:
            namespaceTypes = generatedTypes.setdefault(
                datatype.namespaceUri, OrderedDict())
            namespaceTypes[datatype.name] = datatype
        typeParser.types = generatedTypes
        typeParser.no_builtin = True
        typeParser.existing_types = importedTypes
        typeParser.existing_types_array = set(typesArray)
        return typeParser

    def generateParser(self, existing, infiles , bsdFile):
        self.all_files = []
        # Use the standard schema accompanying the supplied namespace-zero
        # NodeSet, so embedded dictionaries can resolve standard structures.
        schema = Path(__file__).resolve().parent.parent / "schema"
        for xmlfile in existing:
            candidate = Path(xmlfile.name).parent
            if (candidate / "Opc.Ua.Types.bsd").is_file():
                schema = candidate
                break
        import_bsd = ["UA_TYPES#" + str(schema / "Opc.Ua.Types.bsd")]
        type_bsd = []
        for xmlfile in existing:
            if xmlfile.name == "deps/ua-nodeset/DI/Opc.Ua.Di.NodeSet2.xml":
                continue
            nodeset_base = open(xmlfile.name, "rb")
            #fileContent = xmlfile.read()
            fileContent = nodeset_base.read()
            # Remove BOM since the dom parser cannot handle it on python 3 windows
            if fileContent.startswith( codecs.BOM_UTF8 ):
                fileContent = fileContent.lstrip( codecs.BOM_UTF8 )
            fileContent = fileContent.decode("utf-8")

            # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
            fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\\g<1>\\g<2>\\g<3>>", fileContent)

            nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
            if len(nodesets) == 0 or len(nodesets) > 1:
                raise Exception("contains no or more then 1 nodeset")
            nodeset = nodesets[0]
            variableNodes = nodeset.getElementsByTagName("UAVariable")
            for nd in variableNodes:
                if (nd.hasAttribute("SymbolicName") and (re.match(r".*_BinarySchema", nd.attributes["SymbolicName"].nodeValue) or nd.attributes["SymbolicName"].nodeValue == "TypeDictionary_BinarySchema")) or (nd.hasAttribute("ParentNodeId") and not nd.hasAttribute("SymbolicName") and re.fullmatch(r"i=93", nd.attributes["ParentNodeId"].nodeValue)):
                    type_content = nd.getElementsByTagName("Value")[0].getElementsByTagName("ByteString")[0]
                    f = tempfile.NamedTemporaryFile(delete=False, suffix='.bsd')
                    f.write(base64.b64decode(type_content.firstChild.nodeValue))
                    f.flush()
                    self.all_files.append(f.name)
                    f.close()
                    bsd = "UA_TYPES#" + f.name
                    import_bsd.append(bsd)
        for xmlfile in infiles:
            nodeset_base = open(xmlfile.name, "rb")
            #fileContent = xmlfile.read()
            fileContent = nodeset_base.read()
            # Remove BOM since the dom parser cannot handle it on python 3 windows
            if fileContent.startswith( codecs.BOM_UTF8 ):
                fileContent = fileContent.lstrip( codecs.BOM_UTF8 )
            fileContent = fileContent.decode("utf-8")

            # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
            fileContent = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\\g<1>\\g<2>\\g<3>>", fileContent)

            nodesets = dom.parseString(fileContent).getElementsByTagName("UANodeSet")
            if len(nodesets) == 0 or len(nodesets) > 1:
                raise Exception("contains no or more then 1 nodeset")
            nodeset = nodesets[0]
            variableNodes = nodeset.getElementsByTagName("UAVariable")
            for nd in variableNodes:
                if (nd.hasAttribute("SymbolicName") and (re.match(r".*_BinarySchema", nd.attributes["SymbolicName"].nodeValue) or nd.attributes["SymbolicName"].nodeValue == "TypeDictionary_BinarySchema")) or (nd.hasAttribute("ParentNodeId") and not nd.hasAttribute("SymbolicName") and re.fullmatch(r"i=93", nd.attributes["ParentNodeId"].nodeValue)):
                    type_content = nd.getElementsByTagName("Value")[0].getElementsByTagName("ByteString")[0]
                    f = tempfile.NamedTemporaryFile(suffix='.bsd')
                    f.write(base64.b64decode(type_content.firstChild.nodeValue))
                    f.flush()
                    f.seek(0)
                    bf = io.BufferedReader(f)
                    self.all_files.append(f.name)
                    tmp = io.TextIOWrapper(bf, 'UTF-8', newline='\n')
                    tmp.mode = 'r'
                    type_bsd.append(tmp)

        opaque_map = []
        selected_types = []
        type_csv = []
        type_xml = []
        outname = "outname"

        for bsd in bsdFile:
            type_bsd.append(bsd)

        self.parser = CSVBSDTypeParser(opaque_map, selected_types, True, outname,
                                       import_bsd, type_bsd, type_csv, type_xml, self.namespaces)
        self.parser.create_types()

        nodeset_base.close()

    # Kahn's algorithm:
    # https://algocoding.wordpress.com/2015/04/05/topological-sorting-python/
    def sortNodes(self):
        # reverse hastypedefinition references to treat only forward references
        hasTypeDef = NodeId("ns=0;i=40")
        for u in self.nodes.values():
            for ref in u.references:
                if ref.referenceType == hasTypeDef:
                    ref.isForward = not ref.isForward

        # Only hierarchical types...
        relevant_refs = self.getRelevantOrderingReferences()

        # determine in-degree of unfulfilled references
        L = [node for node in self.nodes.values() if node.hidden]  # ordered list of nodes
        R = {node.id: node for node in self.nodes.values() if not node.hidden} # remaining nodes
        in_degree = {id: 0 for id in R.keys()}
        for u in R.values(): # for each node
            for ref in u.references:
                if ref.referenceType not in relevant_refs:
                    continue
                if self.nodes[ref.target].hidden:
                    continue
                if ref.isForward:
                    continue
                in_degree[u.id] += 1

        # Print ReferenceType and DataType nodes first. They may be required even
        # though there is no reference to them. For example if the referencetype is
        # used in a reference, it must exist. A Variable node may point to a
        # DataTypeNode in the datatype attribute and not via an explicit reference.

        Q = [node for node in R.values() if in_degree[node.id] == 0 and
            (isinstance(node, ReferenceTypeNode) or isinstance(node, DataTypeNode))]
        while Q:
            u = Q.pop() # choose node of zero in-degree and 'remove' it from graph
            L.append(u)
            del R[u.id]

            for ref in sorted(u.references, key=lambda r: str(r.target)):
                if ref.referenceType not in relevant_refs:
                    continue
                if self.nodes[ref.target].hidden:
                    continue
                if not ref.isForward:
                    continue
                in_degree[ref.target] -= 1
                if in_degree[ref.target] == 0:
                    Q.append(R[ref.target])

        # Order the remaining nodes
        Q = [node for node in R.values() if in_degree[node.id] == 0]
        while Q:
            u = Q.pop() # choose node of zero in-degree and 'remove' it from graph
            L.append(u)
            del R[u.id]

            for ref in sorted(u.references, key=lambda r: str(r.target)):
                if ref.referenceType not in relevant_refs:
                    continue
                if self.nodes[ref.target].hidden:
                    continue
                if not ref.isForward:
                    continue
                in_degree[ref.target] -= 1
                if in_degree[ref.target] == 0:
                    Q.append(R[ref.target])

        # reverse hastype references
        for u in self.nodes.values():
            for ref in u.references:
                if ref.referenceType == hasTypeDef:
                    ref.isForward = not ref.isForward

        if len(L) != len(self.nodes.values()):
            print(len(L))
            stillOpen = ""
            for id in in_degree:
                if in_degree[id] == 0:
                    continue
                node = self.nodes[id]
                stillOpen += node.browseName.name + "/" + str(node.id) + \
                " = " + str(in_degree[id]) + " " + str(node.references) + "\r\n"
            raise Exception("Node graph is circular on the specified references. "
            "Still open nodes:\r\n" + stillOpen)

        # Done sorting. Replace the OrderedDict of nodes in the nodeset
        self.nodes = OrderedDict([(n.id, n) for n in L])
