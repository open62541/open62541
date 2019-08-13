"""
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""
from datatypes import *
import datetime
import re

import logging

logger = logging.getLogger(__name__)


def generate_boolean_code(value):
    if value:
        return "true"
    return "false"


# Strip invalid characters to create valid C identifiers (variable names etc):
def make_c_identifier(value):
    return re.sub(r'[^\w]', '', value)


# Escape C strings:
def make_c_literal(value):
    return re.sub(r'(?<!\\)"', r'\\"',
                  value.replace('\\', r'\\').replace('"', r'\"').replace('\n', r'\\n').replace('\r', r''))


def split_string_literals(value, split_length=500):
    """
    Split a string literal longer than splitLength into smaller literals.
    E.g. "Some very long text" will be split into "Some ver" "y long te" "xt"
    On VS2008 there is a maximum allowed length of a single string literal.
    """
    value = value.strip()
    if len(value) < split_length or split_length == 0:
        return "\"" + re.sub(r'(?<!\\)"', r'\\"', value) + "\""
    ret = ""
    tmp = value
    while len(tmp) > split_length:
        ret += "\"" + tmp[:split_length].replace('"', r'\"') + "\" "
        tmp = tmp[split_length:]
    ret += "\"" + re.sub(r'(?<!\\)"', r'\\"', tmp) + "\" "
    return ret


def generate_string_code(value, alloc=False):
    value = make_c_literal(value)
    return u"UA_STRING{}({})".format("_ALLOC" if alloc else "", split_string_literals(value))


def generate_xml_element_code(value, alloc=False):
    value = make_c_literal(value)
    return u"UA_XMLELEMENT{}({})".format("_ALLOC" if alloc else "", split_string_literals(value))


def generate_byte_string_code(value, value_name, global_var_code, is_pointer):
    if isinstance(value, str):
        # PY3 returns a byte array for b64decode, while PY2 returns a string.
        # Therefore convert it to bytes
        ascii_array = bytearray()
        ascii_array.extend(value)
        ascii_array = list(ascii_array)
    else:
        ascii_array = list(value)

    ascii_array_str = str(ascii_array).rstrip(']').lstrip('[')
    clean_value_name = re.sub(r"->", "__", re.sub(r"\.", "_", value_name))
    global_var_code.append("static const UA_Byte {cleanValueName}_byteArray[{len}] = {{{data}}};".format(
        len=len(ascii_array), data=ascii_array_str, cleanValueName=clean_value_name
    ))
    # Cast away const with '(UA_Byte *)(void*)(uintptr_t)'
    # since we know that UA_Server_addNode_begin will copy the content
    return "{instance}{accessor}length = {len};\n{instance}{accessor}data = " \
           "(UA_Byte *)(void*)(uintptr_t){cleanValueName}_byteArray;".format(len=len(ascii_array), instance=value_name,
                                                                             cleanValueName=clean_value_name,
                                                                             accessor='->' if is_pointer else '.')


def generate_localized_text_code(value, alloc=False):
    vt = make_c_literal(value.text)
    return u"UA_LOCALIZEDTEXT{}(\"{}\", {})".format("_ALLOC" if alloc else "",
                                                    '' if value.locale is None else value.locale,
                                                    split_string_literals(vt))


def generate_qualified_name_code(value, alloc=False, ):
    vn = make_c_literal(value.name)
    return u"UA_QUALIFIEDNAME{}(ns[{}], {})".format("_ALLOC" if alloc else "",
                                                    str(value.ns), split_string_literals(vn))


def generate_node_id_code(value):
    if not value:
        return "UA_NODEID_NUMERIC(0, 0)"
    if value.i is not None:
        return "UA_NODEID_NUMERIC(ns[%s], %s)" % (value.ns, value.i)
    elif value.s is not None:
        v = make_c_literal(value.s)
        return u"UA_NODEID_STRING(ns[%s], \"%s\")" % (value.ns, v)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")


def generate_expanded_node_id_code(value):
    if value.i is not None:
        return "UA_EXPANDEDNODEID_NUMERIC(ns[%s], %s)" % (str(value.ns), str(value.i))
    elif value.s is not None:
        vs = make_c_literal(value.s)
        return u"UA_EXPANDEDNODEID_STRING(ns[%s], \"%s\")" % (str(value.ns), vs)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")


def generate_date_time_code(value):
    epoch = datetime.datetime.utcfromtimestamp(0)
    msecs_since_epoch = int((value - epoch).total_seconds() * 1000.0)
    return "( (UA_DateTime)(" + str(msecs_since_epoch) + " * UA_DATETIME_MSEC) + UA_DATETIME_UNIX_EPOCH)"


def generate_node_value_code(prepend, node, instance_name, value_name, global_var_code, as_indirect=False):
    if type(node) in [Boolean, Byte, SByte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double]:
        return prepend + "(UA_" + node.__class__.__name__ + ") " + str(node.value) + ";"
    elif type(node) == String:
        return prepend + generate_string_code(node.value, alloc=as_indirect) + ";"
    elif type(node) == XmlElement:
        return prepend + generate_xml_element_code(node.value, alloc=as_indirect) + ";"
    elif type(node) == ByteString:
        # replace whitespaces between tags and remove newlines
        return prepend + "UA_BYTESTRING_NULL;" if not node.value else generate_byte_string_code(
            node.value, value_name, global_var_code, is_pointer=as_indirect)
        # the replacements done here is just for the array form can be workable in C code. It doesn't couses any problem
        # because the core data used here is already in byte form. So, there is no way we disturb it.
    elif type(node) == LocalizedText:
        return prepend + generate_localized_text_code(node, alloc=as_indirect) + ";"
    elif type(node) == NodeId:
        return prepend + generate_node_id_code(node) + ";"
    elif type(node) == ExpandedNodeId:
        return prepend + generate_expanded_node_id_code(node) + ";"
    elif type(node) == DateTime:
        return prepend + generate_date_time_code(node.value) + ";"
    elif type(node) == QualifiedName:
        return prepend + generate_qualified_name_code(node.value, alloc=as_indirect) + ";"
    elif type(node) == StatusCode:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == DiagnosticInfo:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == Guid:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == ExtensionObject:
        if not as_indirect:
            return prepend + "*" + str(instance_name) + ";"
        return prepend + str(instance_name) + ";"
