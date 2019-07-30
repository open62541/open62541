#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Author:  Chris Iatrou (ichrispa@core-vector.net)
"""

from nodes import *
from backend_open62541_datatypes import *
import re
import logging

import sys
if sys.version_info[0] >= 3:
    # strings are already parsed to unicode
    def unicode(s):
        return s

logger = logging.getLogger(__name__)

#################
# Generate Code #
#################


def generate_node_id_printable(node):
    if isinstance(node.id, NodeId):
        code_printable = node.__class__.__name__ + "_" + str(node.id)
    else:
        code_printable = node.__class__.__name__ + "_unknown_nid"

    return re.sub('[^0-9a-z_]+', '_', code_printable.lower())


def generate_node_value_instance_name(node, parent, array_index):
    return generate_node_id_printable(parent) + "_" + str(node.alias) + "_" + str(array_index)


def generate_reference_code(reference):
    if reference.isForward:
        return "retVal |= UA_Server_addReference(server, %s, %s, %s, true);" % \
               (generate_node_id_code(reference.source),
                generate_node_id_code(reference.referenceType),
                generate_expanded_node_id_code(reference.target))
    else:
        return "retVal |= UA_Server_addReference(server, %s, %s, %s, false);" % \
               (generate_node_id_code(reference.source),
                generate_node_id_code(reference.referenceType),
                generate_expanded_node_id_code(reference.target))


def generate_reference_type_node_code(node):
    code = ["UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;"]
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    if node.symmetric:
        code.append("attr.symmetric  = true;")
    if node.inverseName != "":
        code.append("attr.inverseName  = UA_LOCALIZEDTEXT(\"\", \"%s\");" %
                    node.inverseName)
    return code


def generate_object_node_code(node):
    code = ["UA_ObjectAttributes attr = UA_ObjectAttributes_default;"]
    if node.eventNotifier:
        code.append("attr.eventNotifier = true;")
    return code


def set_node_datatype_recursive(node, nodeset):
    if not isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        raise RuntimeError(
            "Node {}: DataType can only be set for VariableNode and VariableTypeNode".format(str(node.id)))

    if node.dataType is not None:
        return

    # If BaseVariableType
    if node.id == NodeId("ns=0;i=62"):
        if node.dataType is None:
            # Set to default BaseDataType
            node.dataType = NodeId("ns=0;i=24")
        return

    if isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        type_def_node = nodeset.get_node_type_definition(node)
        if type_def_node is None:
            # Use the parent type.
            raise RuntimeError(
                "Cannot get node for HasTypeDefinition of VariableNode " + node.browseName.name + " " + str(node.id))

        set_node_datatype_recursive(type_def_node, nodeset)

        node.dataType = type_def_node.dataType
    else:
        # Use the parent type.
        if node.parent is None:
            raise RuntimeError("Parent node not defined for " + node.browseName.name + " " + str(node.id))

        set_node_datatype_recursive(node.parent, nodeset)
        node.dataType = node.parent.dataType


def set_node_value_rank_recursive(node, nodeset):
    if not isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        raise RuntimeError(
            "Node {}: ValueRank can only be set for VariableNode and VariableTypeNode".format(str(node.id)))

    if node.valueRank is not None:
        return

    # If BaseVariableType
    if node.id == NodeId("ns=0;i=62"):
        if node.valueRank is None:
            # BaseVariableType always has -2
            node.valueRank = -2
        return

    if isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        type_def_node = nodeset.get_node_type_definition(node)
        if type_def_node is None:
            # Use the parent type.
            raise RuntimeError(
                "Cannot get node for HasTypeDefinition of VariableNode " + node.browseName.name + " " + str(node.id))
        if not isinstance(type_def_node, VariableTypeNode):
            raise RuntimeError("Node {} ({}) has an invalid type definition. {} is not a VariableType node.".format(
                str(node.id), node.browseName.name, str(type_def_node.id)))

        set_node_value_rank_recursive(type_def_node, nodeset)

        if type_def_node.valueRank is not None and type_def_node.valueRank > -1:
            node.valueRank = type_def_node.valueRank
        else:
            # Default value
            node.valueRank = -1
    else:
        # Check if parent node limits the value rank
        set_node_value_rank_recursive(node.parent, nodeset)

        if node.parent.valueRank is not None and node.parent.valueRank > -1:
            node.valueRank = node.parent.valueRank
        else:
            # Default value
            node.valueRank = -1


def generate_common_variable_code(node, nodeset):
    code = []
    code_cleanup = []
    code_global = []

    if node.valueRank is None:
        # Set the constrained value rank from the type/parent node
        set_node_value_rank_recursive(node, nodeset)
        code.append("/* Value rank inherited */")

    code.append("attr.valueRank = %d;" % node.valueRank)
    if node.valueRank > 0:
        code.append("attr.arrayDimensionsSize = %d;" % node.valueRank)
        code.append("UA_UInt32 arrayDimensions[{}];".format(node.valueRank))
        if len(node.arrayDimensions) == node.valueRank:
            for idx, v in enumerate(node.arrayDimensions):
                code.append("arrayDimensions[{}] = {};".format(idx, int(str(v))))
        else:
            for dim in range(0, node.valueRank):
                code.append("arrayDimensions[{}] = 0;".format(dim))
        code.append("attr.arrayDimensions = &arrayDimensions[0];")

    if node.dataType is None:
        # Inherit the datatype from the HasTypeDefinition reference, as stated in the OPC UA Spec:
        # 6.4.2
        # "Instances inherit the initial values for the Attributes that they have in common with the
        # TypeDefinitionNode from which they are instantiated, with the exceptions of the NodeClass and
        # NodeId."
        set_node_datatype_recursive(node, nodeset)
        code.append("/* DataType inherited */")

    data_type_node = nodeset.get_base_data_type(nodeset.get_data_type_node(node.dataType))

    if data_type_node is None:
        raise RuntimeError("Cannot get BaseDataType for dataType : " + str(
            node.dataType) + " of node " + node.browseName.name + " " + str(node.id))

    code.append("attr.dataType = %s;" % generate_node_id_code(node.dataType))

    if data_type_node.is_encodable():
        if node.value is not None:
            [code1, code_cleanup1, code_global1] = generate_value_code(node.value, nodeset.nodes[node.id],
                                                                       nodeset)
            code += code1
            code_cleanup += code_cleanup1
            code_global += code_global1
            # #1978 Variant arrayDimensions are only required to properly decode multidimensional arrays
            # (valueRank > 1) from data stored as one-dimensional array of arrayLength elements.
            # One-dimensional arrays are already completely defined by arraylength attribute so setting
            # also arrayDimensions, even if not explicitly forbidden, can confuse clients
            if node.valueRank is not None and 1 < node.valueRank == len(node.arrayDimensions) and len(
                    node.value.value) > 0:
                num_elements = 1
                has_zero = False
                for v in node.arrayDimensions:
                    dim = int(unicode(v))
                    if dim > 0:
                        num_elements = num_elements * dim
                    else:
                        has_zero = True
                if has_zero is False and len(node.value.value) == num_elements:
                    code.append("attr.value.arrayDimensionsSize = attr.arrayDimensionsSize;")
                    code.append("attr.value.arrayDimensions = attr.arrayDimensions;")
    elif node.value is not None:
        raise RuntimeError("Cannot encode data_type_node: " + data_type_node.browseName.name + " for value of node " +
                           node.browseName.name + " " + str(node.id))

    return [code, code_cleanup, code_global]


def generate_variable_node_code(node, nodeset):
    code = []
    code_cleanup = []
    code_global = []
    code.append("UA_VariableAttributes attr = UA_VariableAttributes_default;")
    if node.historizing:
        code.append("attr.historizing = true;")
    code.append("attr.minimumSamplingInterval = %f;" % node.minimumSamplingInterval)
    code.append("attr.userAccessLevel = %d;" % node.userAccessLevel)
    code.append("attr.accessLevel = %d;" % node.accessLevel)
    # in order to be compatible with mostly OPC UA client
    # force valueRank = -1 for scalar VariableNode
    if node.valueRank == -2 and node.value is not None and len(node.value.value) == 1:
        node.valueRank = -1
    [code1, code_cleanup1, code_global1] = generate_common_variable_code(node, nodeset)
    code += code1
    code_cleanup += code_cleanup1
    code_global += code_global1

    return [code, code_cleanup, code_global]


def generate_variable_type_node_code(node, nodeset):
    code = []
    code_cleanup = []
    code_global = []
    code.append("UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;")
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    [code1, code_cleanup1, code_global1] = generate_common_variable_code(node, nodeset)
    code += code1
    code_cleanup += code_cleanup1
    code_global += code_global1

    return [code, code_cleanup, code_global]


def lower_first_char(input_string):
    return input_string[0].lower() + input_string[1:]


def generate_extension_object_subtype_code(node, parent, nodeset, global_var_code,
                                           instance_name=None, is_array_element=False):
    code = [""]
    code_cleanup = [""]

    logger.debug("Building extensionObject for " + str(parent.id))
    logger.debug("Value    " + str(node.value))
    logger.debug("Encoding " + str(node.encodingRule))

    type_browse_node = make_c_identifier(nodeset.get_data_type_node(parent.dataType).browseName.name)
    # TODO: review this
    if type_browse_node == "NumericRange":
        # in the stack we define a separate structure for the numeric range, but
        # the value itself is just a string
        type_browse_node = "String"

    type_string = "UA_" + type_browse_node
    if instance_name is None:
        instance_name = generate_node_value_instance_name(node, parent, 0)
        code.append("UA_STACKARRAY(" + type_string + ", " + instance_name + ", 1);")
    type_arr = nodeset.get_data_type_node(parent.dataType).typesArray
    type_string = nodeset.get_data_type_node(parent.dataType).browseName.name.upper()
    type_array_string = type_arr + "[" + type_arr + "_" + type_string + "]"
    code.append("UA_init({ref}{instance_name}, &{type_array_string});".format(ref="&" if is_array_element else "",
                                                                              instance_name=instance_name,
                                                                              type_array_string=type_array_string))

    # Assign data to the struct contents
    # Track the encoding rule definition to detect arrays and/or ExtensionObjects
    enc_field_idx = 0
    for subv in node.value:
        enc_field = node.encodingRule[enc_field_idx]
        enc_field_idx = enc_field_idx + 1
        member_name = lower_first_char(enc_field[0])

        # Check if this is an array
        accessor = "." if is_array_element else "->"

        if isinstance(subv, list):
            if len(subv) == 0:
                continue
            logger.info("ExtensionObject contains array")
            member_name = lower_first_char(enc_field[0])
            enc_type_string = "UA_" + subv[0].__class__.__name__
            instance_name_safe = make_c_identifier(instance_name)
            code.append("UA_STACKARRAY(" + enc_type_string + ", " + instance_name_safe + "_" + member_name +
                        ", {0});".format(len(subv)))
            enc_type_arr = nodeset.get_data_type_node(subv[0].__class__.__name__).typesArray
            enc_type_array_string = enc_type_arr + "[" + enc_type_arr + "_" + subv[0].__class__.__name__.upper() + "]"
            code.append("UA_init({instance_name}, &{type_array_string});".format(
                instance_name=instance_name_safe + "_" + member_name,
                type_array_string=enc_type_array_string))

            sub_array_idx = 0
            for val in subv:
                code.append(generate_node_value_code(
                    instance_name_safe + "_" + member_name + "[" + str(sub_array_idx) + "]" + " = ", val, instance_name,
                    instance_name + "_gehtNed_member", global_var_code, as_indirect=False))
                sub_array_idx = sub_array_idx + 1
            code.append(instance_name + accessor + member_name + " = " + instance_name_safe + "_" + member_name + ";")
            continue
        else:
            logger.debug(
                "Encoding of field " + member_name + " is " + str(subv.encodingRule) + "defined by " + str(enc_field))

        value_name = None
        if subv.valueRank is None or subv.valueRank == 0:
            if not subv.is_none():
                # Some values can be optional
                value_name = instance_name + accessor + member_name
                code.append(generate_node_value_code(value_name + " = ",
                                                     subv, instance_name, value_name, global_var_code,
                                                     as_indirect=False))
        else:
            member_name = lower_first_char(enc_field[0])
            code.append(generate_node_value_code(instance_name + accessor + member_name + "Size = ", subv,
                                                 instance_name, value_name, global_var_code, as_indirect=False))

    if not is_array_element:
        code.append("UA_Variant_setScalar(&attr.value, " + instance_name + ", &" + type_array_string + ");")

    return [code, code_cleanup]


def get_type_browse_name(data_type_node):
    type_browse_name = make_c_identifier(data_type_node.browseName.name)
    # TODO: review this
    if type_browse_name == "NumericRange":
        # in the stack we define a separate structure for the numeric range, but
        # the value itself is just a string
        type_browse_name = "String"
    return type_browse_name


def get_types_array_for_value(nodeset, value):
    type_node = nodeset.get_node_by_browse_name(value.__class__.__name__)
    if type_node is None or value.isInternal:
        types_array = "UA_TYPES"
    else:
        types_array = type_node.typesArray
    type_name = make_c_identifier(value.__class__.__name__.upper())
    return "&" + types_array + "[" + types_array + "_" + type_name + "]"


def is_array_variable_node(node, parent_node):
    ret = parent_node.valueRank is not None
    ret &= parent_node.valueRank != -1
    ret &= parent_node.valueRank >= 0 or (
                len(node.value) > 1 and (parent_node.valueRank != -2 or parent_node.valueRank != -3))
    return ret


def generate_value_code(node, parent_node, nodeset, bootstrapping=True):
    code = []
    code_cleanup = []
    code_global = []
    value_name = generate_node_id_printable(parent_node) + "_variant_DataContents"

    # node.value either contains a list of multiple identical BUILTINTYPES, or it
    # contains a single builtintype (which may be a container); choose if we need
    # to create an array or a single variable.
    # Note that some genious defined that there are arrays of size 1, which are
    # distinctly different then a single value, so we need to check that as well
    # Semantics:
    # -3: Scalar or 1-dim
    # -2: Scalar or x-dim | x>0
    # -1: Scalar
    #  0: x-dim | x>0
    #  n: n-dim | n>0
    if len(node.value) == 0:
        return ["", "", ""]
    if not isinstance(node.value[0], Value):
        return ["", "", ""]

    data_type_node = nodeset.get_data_type_node(parent_node.dataType)

    if is_array_variable_node(node, parent_node):
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if isinstance(node.value[0], Guid):
            logger.warning("Don't know how to print array of GUID in node " + str(parent_node.id))
        elif isinstance(node.value[0], DiagnosticInfo):
            logger.warning("Don't know how to print array of DiagnosticInfo in node " + str(parent_node.id))
        elif isinstance(node.value[0], StatusCode):
            logger.warning("Don't know how to print array of StatusCode in node " + str(parent_node.id))
        else:
            if isinstance(node.value[0], ExtensionObject):
                code.append(
                    "UA_" + get_type_browse_name(data_type_node) + " " + value_name + "[" + str(len(node.value)) + "];")
                for idx, v in enumerate(node.value):
                    logger.debug("Building extObj array index " + str(idx))
                    instance_name = value_name + "[" + str(idx) + "]"
                    [code1, code_cleanup1] = generate_extension_object_subtype_code(v, parent=parent_node,
                                                                                    nodeset=nodeset,
                                                                                    global_var_code=code_global,
                                                                                    instance_name=instance_name,
                                                                                    is_array_element=True)
                    code = code + code1
                    code_cleanup = code_cleanup + code_cleanup1
            else:
                code.append(
                    "UA_" + node.value[0].__class__.__name__ + " " + value_name + "[" + str(len(node.value)) + "];")
                for idx, v in enumerate(node.value):
                    instance_name = generate_node_value_instance_name(v, parent_node, idx)
                    code.append(generate_node_value_code(
                        value_name + "[" + str(idx) + "] = ", v, instance_name, value_name, code_global))
            code.append("UA_Variant_setArray(&attr.value, &" + value_name +
                        ", (UA_Int32) " + str(len(node.value)) + ", " + "&" +
                        data_type_node.typesArray + "[" + data_type_node.typesArray + "_" +
                        get_type_browse_name(data_type_node).upper() + "]);")
    # scalar value
    else:
        # User the following strategy for all directly mappable values a la 'UA_Type MyInt = (UA_Type) 23;'
        if isinstance(node.value[0], Guid):
            logger.warning("Don't know how to print scalar GUID in node " + str(parent_node.id))
        elif isinstance(node.value[0], DiagnosticInfo):
            logger.warning("Don't know how to print scalar DiagnosticInfo in node " + str(parent_node.id))
        elif isinstance(node.value[0], StatusCode):
            logger.warning("Don't know how to print scalar StatusCode in node " + str(parent_node.id))
        else:
            # The following strategy applies to all other types, in particular strings and numerics.
            if isinstance(node.value[0], ExtensionObject):
                [code1, code_cleanup1] = generate_extension_object_subtype_code(node.value[0], parent=parent_node,
                                                                                nodeset=nodeset,
                                                                                global_var_code=code_global,
                                                                                is_array_element=False)
                code = code + code1
                code_cleanup = code_cleanup + code_cleanup1
            instance_name = generate_node_value_instance_name(node.value[0], parent_node, 0)
            if not node.value[0].is_none() and not(isinstance(node.value[0], ExtensionObject)):
                code.append("UA_" + node.value[0].__class__.__name__ + " *" + value_name + " =  UA_" + node.value[
                    0].__class__.__name__ + "_new();")
                code.append("if (!" + value_name + ") return UA_STATUSCODE_BADOUTOFMEMORY;")
                code.append("UA_" + node.value[0].__class__.__name__ + "_init(" + value_name + ");")
                code.append(generate_node_value_code("*" + value_name + " = ", node.value[0], instance_name, value_name,
                                                     code_global, as_indirect=True))
                code.append(
                        "UA_Variant_setScalar(&attr.value, " + value_name + ", " +
                        get_types_array_for_value(nodeset, node.value[0]) + ");")
                if node.value[0].__class__.__name__ == "ByteString":
                    # The data is on the stack, not heap, so we can not delete the ByteString
                    code_cleanup.append("{}->data = NULL;".format(value_name))
                    code_cleanup.append("{}->length = 0;".format(value_name))
                code_cleanup.append("UA_{0}_delete({1});".format(
                    node.value[0].__class__.__name__, value_name))
    return [code, code_cleanup, code_global]


def generate_method_node_code(node):
    code = ["UA_MethodAttributes attr = UA_MethodAttributes_default;"]
    if node.executable:
        code.append("attr.executable = true;")
    if node.userExecutable:
        code.append("attr.userExecutable = true;")
    return code


def generate_object_type_node_code(node):
    code = ["UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;"]
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    return code


def generate_data_type_node_code(node):
    code = ["UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;"]
    if node.isAbstract:
        code.append("attr.isAbstract = true;")
    return code


def generate_view_node_code(node):
    code = ["UA_ViewAttributes attr = UA_ViewAttributes_default;"]
    if node.containsNoLoops:
        code.append("attr.containsNoLoops = true;")
    code.append("attr.eventNotifier = (UA_Byte)%s;" % str(node.eventNotifier))
    return code


def generate_subtype_of_definition_code(node):
    for ref in node.inverseReferences:
        # 45 = HasSubtype
        if ref.referenceType.i == 45:
            return generate_node_id_code(ref.target)
    return "UA_NODEID_NULL"


def generate_node_code_begin(node, nodeset, code_global):
    code = []
    code_cleanup = []
    code.append("UA_StatusCode retVal = UA_STATUSCODE_GOOD;")

    # Attributes
    if isinstance(node, ReferenceTypeNode):
        code.extend(generate_reference_type_node_code(node))
    elif isinstance(node, ObjectNode):
        code.extend(generate_object_node_code(node))
    elif isinstance(node, VariableNode) and not isinstance(node, VariableTypeNode):
        [code1, code_cleanup1, code_global1] = generate_variable_node_code(node, nodeset)
        code.extend(code1)
        code_cleanup.extend(code_cleanup1)
        code_global.extend(code_global1)
    elif isinstance(node, VariableTypeNode):
        [code1, code_cleanup1, code_global1] = generate_variable_type_node_code(node, nodeset)
        code.extend(code1)
        code_cleanup.extend(code_cleanup1)
        code_global.extend(code_global1)
    elif isinstance(node, MethodNode):
        code.extend(generate_method_node_code(node))
    elif isinstance(node, ObjectTypeNode):
        code.extend(generate_object_type_node_code(node))
    elif isinstance(node, DataTypeNode):
        code.extend(generate_data_type_node_code(node))
    elif isinstance(node, ViewNode):
        code.extend(generate_view_node_code(node))
    if node.displayName is not None:
        code.append("attr.displayName = " + generate_localized_text_code(node.displayName, alloc=False) + ";")
    if node.description is not None:
        code.append("#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS")
        code.append("attr.description = " + generate_localized_text_code(node.description, alloc=False) + ";")
        code.append("#endif")
    if node.writeMask is not None:
        code.append("attr.writeMask = %d;" % node.writeMask)
    if node.userWriteMask is not None:
        code.append("attr.userWriteMask = %d;" % node.userWriteMask)

    # AddNodes call
    code.append("retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_{},".format(
        make_c_identifier(node.__class__.__name__.upper().replace("NODE", ""))))
    code.append(generate_node_id_code(node.id) + ",")
    code.append(generate_node_id_code(node.parent.id if node.parent else NodeId()) + ",")
    code.append(generate_node_id_code(node.parentReference.id if node.parent else NodeId()) + ",")
    code.append(generate_qualified_name_code(node.browseName) + ",")
    if isinstance(node, VariableNode) or isinstance(node, ObjectNode):
        type_def_ref = node.pop_type_def()
        code.append(generate_node_id_code(type_def_ref.target) + ",")
    else:
        code.append(" UA_NODEID_NULL,")
    code.append("(const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_{}ATTRIBUTES],NULL, NULL);".format(
        make_c_identifier(node.__class__.__name__.upper().replace("NODE", ""))))
    code.extend(code_cleanup)

    return "\n".join(code)


def generate_node_code_finish(node):
    code = []

    if isinstance(node, MethodNode):
        code.append("UA_Server_addMethodNode_finish(server, ")
    else:
        code.append("UA_Server_addNode_finish(server, ")
    code.append(generate_node_id_code(node.id))

    if isinstance(node, MethodNode):
        code.append(", NULL, 0, NULL, 0, NULL);")
    else:
        code.append(");")

    return "\n".join(code)
