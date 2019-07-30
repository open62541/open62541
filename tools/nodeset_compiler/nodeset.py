#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Author:  Chris Iatrou (ichrispa@core-vector.net)
"""

from __future__ import print_function
import sys
import xml.dom.minidom as dom
import logging
import codecs
import re
from datatypes import NodeId, value_is_internal_type
from nodes import (VariableNode, ObjectNode, MethodNode, ObjectTypeNode, VariableTypeNode, DataTypeNode,
                   ReferenceTypeNode, Reference)
from opaque_type_mapping import opaque_type_mapping

__all__ = ['NodeSet', 'get_sub_types_of']

logger = logging.getLogger(__name__)

if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s
    string_types = str
else:
    string_types = basestring

####################
# Helper Functions #
####################

has_subtype = NodeId("ns=0;i=45")


def get_sub_types_of(nodeset, node, skip_nodes=None):
    if skip_nodes is None:
        skip_nodes = []
    if node in skip_nodes:
        return []
    # TODO: What is re? naming
    re = set()
    re.add(node)
    for ref in node.references:
        if ref.referenceType == has_subtype:
            skip_all = set()
            skip_all.update(skip_nodes)
            skip_all.update(re)
            if ref.source == node.id and ref.isForward:
                re.update(get_sub_types_of(nodeset, nodeset.nodes[ref.target], skip_nodes=skip_all))
            elif ref.target == node.id and not ref.isForward:
                re.update(get_sub_types_of(nodeset, nodeset.nodes[ref.source], skip_nodes=skip_all))
    return re


def extract_namespaces(xmlfile):
    # Extract a list of namespaces used. The first namespace is always
    # "http://opcfoundation.org/UA/". minidom gobbles up
    # <NamespaceUris></NamespaceUris> elements, without a decent way to reliably
    # access this dom2 <uri></uri> elements (only attribute xmlns= are accessible
    # using minidom). We need them for dereferencing though... This function
    # attempts to do just that.

    namespaces = ["http://opcfoundation.org/UA/"]
    infile = codecs.open(xmlfile.name, encoding='utf-8')
    found_uris = False
    namespace_line = ""
    for line in infile:
        if "<namespaceuris>" in line.lower():
            found_uris = True
        elif "</namespaceuris>" in line.lower():
            namespace_line = namespace_line + line
            break
        if found_uris:
            namespace_line = namespace_line + line

    if len(namespace_line) > 0:
        ns = dom.parseString(namespace_line).getElementsByTagName("NamespaceUris")
        for uri in ns[0].childNodes:
            if uri.nodeType != uri.ELEMENT_NODE:
                continue
            if uri.firstChild.data in namespaces:
                continue
            namespaces.append(uri.firstChild.data)
    infile.close()
    return namespaces


def build_alias_list(xml_element):
    """Parses the <Alias> XML Element present in must XML NodeSet definitions.
       Contents the Alias element are stored in a dictionary for further
       dereferencing during pointer linkage (see linkOpenPointer())."""
    aliases = {}
    for al in xml_element.childNodes:
        if al.nodeType == al.ELEMENT_NODE:
            if al.hasAttribute("Alias"):
                aliasst = al.getAttribute("Alias")
                aliasnd = unicode(al.firstChild.data)
                aliases[aliasst] = aliasnd
    return aliases


class NodeSet(object):
    """ This class handles parsing XML description of namespaces, instantiating
        nodes, linking references, graphing the namespace and compiling a binary
        representation.

        Note that nodes assigned to this class are not restricted to having a
        single namespace ID. This class represents the entire physical address
        space of the binary representation and all nodes that are to be included
        in that segment of memory.
    """

    def __init__(self):
        self.nodes = {}
        self.aliases = {}
        self.namespace_urls = ["http://opcfoundation.org/UA/"]
        self.namespace_mapping = {}

    def sanitize(self):
        for n in self.nodes.values():
            if not n.sanitize():
                raise Exception("Failed to sanitize node " + str(n))

        # Sanitize reference consistency
        for n in self.nodes.values():
            for ref in n.references:
                if not ref.source == n.id:
                    raise Exception("Reference " + str(ref) + " has an invalid source")
                if ref.referenceType not in self.nodes:
                    raise Exception("Reference " + str(ref) + " has an unknown reference type")
                if ref.target not in self.nodes:
                    raise Exception("Reference " + str(ref) + " has an unknown target")

    def add_namespace_url(self, namespace_url):
        if namespace_url not in self.namespace_urls:
            self.namespace_urls.append(namespace_url)

    def create_namespace_mapping(self, orig_namespaces):
        """Creates a dict that maps from the nsindex in the original nodeset to the
           nsindex in the combined nodeset"""
        m = {}
        for index, name in enumerate(orig_namespaces):
            m[index] = self.namespace_urls.index(name)
        return m

    def get_node_by_browse_name(self, id_string):
        return next((n for n in self.nodes.values() if id_string == n.browseName.name), None)

    def get_node_by_id(self, namespace, id):
        node_id = NodeId()
        node_id.ns = namespace
        node_id.i = id
        return self.nodes[node_id]

    def get_root(self):
        return self.get_node_by_browse_name("Root")

    def create_node(self, xml_element, model_uri, hidden=False):
        node_type = xml_element.localName.lower()
        if node_type[:2] == "ua":
            node_type = node_type[2:]

        node = None
        if node_type == 'variable':
            node = VariableNode(xml_element)
        if node_type == 'object':
            node = ObjectNode(xml_element)
        if node_type == 'method':
            node = MethodNode(xml_element)
        if node_type == 'objecttype':
            node = ObjectTypeNode(xml_element)
        if node_type == 'variabletype':
            node = VariableTypeNode(xml_element)
        if node_type == 'methodtype':
            node = MethodNode(xml_element)
        if node_type == 'datatype':
            node = DataTypeNode(xml_element)
        if node_type == 'referencetype':
            node = ReferenceTypeNode(xml_element)

        if node is None:
            return None

        node.modelUri = model_uri
        node.hidden = hidden
        return node

    def hide_node(self, node_id, hidden=True):
        if node_id not in self.nodes:
            return False
        node = self.nodes[node_id]
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

    def add_node_set(self, xml_file, hidden=False, types_array="UA_TYPES"):
        # Extract NodeSet DOM

        file_content = xml_file.read()
        # Remove BOM since the dom parser cannot handle it on python 3 windows
        if file_content.startswith(codecs.BOM_UTF8):
            file_content = file_content.lstrip(codecs.BOM_UTF8)
        if sys.version_info >= (3, 0):
            file_content = file_content.decode("utf-8")

        # Remove the uax namespace from tags. UaModeler adds this namespace to some elements
        file_content = re.sub(r"<([/]?)uax:(.+?)([/]?)>", "<\g<1>\g<2>\g<3>>", file_content)

        nodesets = dom.parseString(file_content).getElementsByTagName("UANodeSet")
        if len(nodesets) == 0 or len(nodesets) > 1:
            raise Exception(self, self.originXML + " contains no or more then 1 nodeset")
        nodeset = nodesets[0]

        # Extract the model_uri
        try:
            model_tag = nodeset.getElementsByTagName("Models")[0].getElementsByTagName("Model")[0]
            model_uri = model_tag.attributes["ModelUri"].nodeValue
        except Exception:
            # Ignore exception and try to use namespace array
            model_uri = None

        # Create the namespace mapping
        orig_namespaces = extract_namespaces(xml_file)  # List of namespaces used in the xml file
        if model_uri is None and len(orig_namespaces) > 1:
            model_uri = orig_namespaces[1]

        if model_uri is None:
            raise Exception(self, self.originXML +
                            " does not define the nodeset URI in Models/Model/ModelUri or NamespaceUris array.")

        for namespace_url in orig_namespaces:
            self.add_namespace_url(namespace_url)
        self.namespace_mapping[model_uri] = self.create_namespace_mapping(orig_namespaces)

        # Extract the aliases
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            ndtype = nd.localName.lower()
            if 'aliases' in ndtype:
                self.aliases = self.merge_dicts(self.aliases, build_alias_list(nd))

        # Instantiate nodes
        newnodes = []
        for nd in nodeset.childNodes:
            if nd.nodeType != nd.ELEMENT_NODE:
                continue
            node = self.create_node(nd, model_uri, hidden)
            if not node:
                continue
            node.replace_aliases(self.aliases)
            node.replace_namespaces(self.namespace_mapping[model_uri])
            node.typesArray = types_array

            # Add the node the the global dict
            if node.id in self.nodes:
                raise Exception("XMLElement with duplicate ID " + str(node.id))
            self.nodes[node.id] = node
            newnodes.append(node)

    def get_binary_encoding_id_for_node(self, node_id):
        """
        The node should have a 'HasEncoding' forward reference which points to the encoding ids.
        These can be XML Encoding or Binary Encoding. Therefore we also need to check if the SymbolicName
        of the target node is "DefaultBinary"
        """
        node = self.nodes[node_id]
        for ref in node.references:
            if ref.referenceType.ns == 0 and ref.referenceType.i == 38:
                ref_node = self.nodes[ref.target]
                if ref_node.symbolicName.value == "DefaultBinary":
                    return ref.target
        raise Exception("No DefaultBinary encoding defined for node " + str(node_id))

    def build_encoding_rules(self):
        """ Calls buildEncoding() for all DataType nodes (opcua_node_dataType_t).

            No return value
        """
        stat = {True: 0, False: 0}
        for n in self.nodes.values():
            if isinstance(n, DataTypeNode):
                n.build_encoding(self)
                stat[n.is_encodable()] = stat[n.is_encodable()] + 1
        logger.debug("Type definitions built/passed: " + str(stat))

    def allocate_variables(self):
        for n in self.nodes.values():
            if isinstance(n, VariableNode):
                n.allocate_value(self)

    def get_base_data_type(self, node):
        if node is None:
            return None
        if node.browseName.name not in opaque_type_mapping:
            return node
        for ref in node.references:
            if ref.isForward:
                continue
            if ref.referenceType.i == 45:
                return self.get_base_data_type(self.nodes[ref.target])
        return node

    def get_node_type_definition(self, node):
        for ref in node.references:
            # 40 = HasTypeDefinition
            if ref.referenceType.i == 40 and ref.isForward:
                return self.nodes[ref.target]
        return None

    def get_data_type_node(self, data_type):
        if isinstance(data_type, string_types):
            if not value_is_internal_type(data_type):
                logger.error("Not a valid dataType string: " + data_type)
                return None
            return self.nodes[NodeId(self.aliases[data_type])]
        if isinstance(data_type, NodeId):
            if data_type.i == 0:
                return None
            data_type_node = self.nodes[data_type]
            if not isinstance(data_type_node, DataTypeNode):
                logger.error("Node id " + str(data_type) + " is not reference a valid dataType.")
                return None
            if not data_type_node.is_encodable():
                logger.warning("DataType " + str(data_type_node.browseName) + " is not encodable.")
            return data_type_node
        return None

    def get_relevant_ordering_references(self):
        relevant_types = set()
        relevant_types.update(get_sub_types_of(self, self.get_node_by_browse_name("HierarchicalReferences"), []))
        relevant_types.update(get_sub_types_of(self, self.get_node_by_browse_name("HasEncoding"), []))
        relevant_types.update(get_sub_types_of(self, self.get_node_by_browse_name("HasTypeDefinition"), []))
        return list(map(lambda x: x.id, relevant_types))

    def add_inverse_references(self):
        # Ensure that every reference has an inverse reference in the target
        for u in self.nodes.values():
            for ref in u.references:
                back = Reference(ref.target, ref.referenceType, ref.source, not ref.isForward)
                self.nodes[ref.target].references.add(back)  # ref set does not make a duplicate entry

    def set_node_parent(self):
        parent_ref_types = get_sub_types_of(self, self.get_node_by_browse_name("HierarchicalReferences"))
        parent_ref_types = list(map(lambda x: x.id, parent_ref_types))

        for node in self.nodes.values():
            parent_ref = node.get_parent_reference(parent_ref_types)
            if parent_ref is not None:
                node.parent = self.nodes[parent_ref.target]
                node.parentReference = self.nodes[parent_ref.referenceType]
