from datatypes import *
import datetime
import re

import logging

logger = logging.getLogger(__name__)

def generateBooleanCode(value):
    if value:
        return "true"
    return "false"

# Strip invalid characters to create valid C identifiers (variable names etc):
def makeCIdentifier(value):
    return re.sub(r'[^\w]', '', value)

# Escape C strings:
def makeCLiteral(value):
    return re.sub(r'(?<!\\)"', r'\\"', value.replace('\\', r'\\').replace('"', r'\"').replace('\n', r'\\n').replace('\r', r''))

def splitStringLiterals(value, splitLength=500):
    """
    Split a string literal longer than splitLength into smaller literals.
    E.g. "Some very long text" will be split into "Some ver" "y long te" "xt"
    On VS2008 there is a maximum allowed length of a single string literal.
    """
    value = value.strip()
    if len(value) < splitLength or splitLength == 0:
        return "\"" + re.sub(r'(?<!\\)"', r'\\"', value) + "\""
    ret = ""
    tmp = value
    while len(tmp) > splitLength:
        ret += "\"" + tmp[:splitLength].replace('"', r'\"') + "\" "
        tmp = tmp[splitLength:]
    ret += "\"" + re.sub(r'(?<!\\)"', r'\\"', tmp) + "\" "
    return ret

def generateStringCode(value, alloc=False):
    value = makeCLiteral(value)
    return u"UA_STRING{}({})".format("_ALLOC" if alloc else "", splitStringLiterals(value))

def generateXmlElementCode(value, alloc=False):
    value = makeCLiteral(value)
    return u"UA_XMLELEMENT{}({})".format("_ALLOC" if alloc else "", splitStringLiterals(value))

def generateByteStringCode(value, valueName, global_var_code, isPointer):
    if isinstance(value, str):
        # PY3 returns a byte array for b64decode, while PY2 returns a string.
        # Therefore convert it to bytes
        asciiarray = bytearray()
        asciiarray.extend(value)
        asciiarray = list(asciiarray)
    else:
        asciiarray = list(value)

    asciiarraystr = str(asciiarray).rstrip(']').lstrip('[')
    cleanValueName = re.sub(r"->", "__", re.sub(r"\.", "_", valueName))
    global_var_code.append("static const UA_Byte {cleanValueName}_byteArray[{len}] = {{{data}}};".format(
        len=len(asciiarray), data=asciiarraystr, cleanValueName=cleanValueName
    ))
    # Cast away const with '(UA_Byte *)(void*)(uintptr_t)' since we know that UA_Server_addNode_begin will copy the content
    return "{instance}{accessor}length = {len};\n{instance}{accessor}data = (UA_Byte *)(void*)(uintptr_t){cleanValueName}_byteArray;"\
                                                .format(len=len(asciiarray), instance=valueName, cleanValueName=cleanValueName,
                                                        accessor='->' if isPointer else '.')

def generateLocalizedTextCode(value, alloc=False):
    vt = makeCLiteral(value.text)
    return u"UA_LOCALIZEDTEXT{}(\"{}\", {})".format("_ALLOC" if alloc else "", '' if value.locale is None else value.locale,
                                                   splitStringLiterals(vt))

def generateQualifiedNameCode(value, alloc=False,):
    vn = makeCLiteral(value.name)
    return u"UA_QUALIFIEDNAME{}(ns[{}], {})".format("_ALLOC" if alloc else "",
                                                     str(value.ns), splitStringLiterals(vn))

def generateNodeIdCode(value):
    if not value:
        return "UA_NODEID_NUMERIC(0, 0)"
    if value.i != None:
        return "UA_NODEID_NUMERIC(ns[%s], %s)" % (value.ns, value.i)
    elif value.s != None:
        v = makeCLiteral(value.s)
        return u"UA_NODEID_STRING(ns[%s], \"%s\")" % (value.ns, v)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateExpandedNodeIdCode(value):
    if value.i != None:
        return "UA_EXPANDEDNODEID_NUMERIC(ns[%s], %s)" % (str(value.ns), str(value.i))
    elif value.s != None:
        vs = makeCLiteral(value.s)
        return u"UA_EXPANDEDNODEID_STRING(ns[%s], \"%s\")" % (str(value.ns), vs)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateDateTimeCode(value):
    epoch = datetime.datetime.utcfromtimestamp(0)
    mSecsSinceEpoch = int((value - epoch).total_seconds() * 1000.0)
    return "( (UA_DateTime)(" + str(mSecsSinceEpoch) + " * UA_DATETIME_MSEC) + UA_DATETIME_UNIX_EPOCH)"

def generateNodeValueCode(prepend , node, instanceName, valueName, global_var_code, asIndirect=False):
    if type(node) in [Boolean, Byte, SByte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double]:
        return prepend + "(UA_" + node.__class__.__name__ + ") " + str(node.value) + ";"
    elif type(node) == String:
        return prepend + generateStringCode(node.value, alloc=asIndirect) + ";"
    elif type(node) == XmlElement:
        return prepend + generateXmlElementCode(node.value, alloc=asIndirect) + ";"
    elif type(node) == ByteString:
        # replace whitespaces between tags and remove newlines
        return prepend + "UA_BYTESTRING_NULL;" if not node.value else generateByteStringCode(
            node.value, valueName, global_var_code, isPointer=asIndirect)
        # the replacements done here is just for the array form can be workable in C code. It doesn't couses any problem
        # because the core data used here is already in byte form. So, there is no way we disturb it.
    elif type(node) == LocalizedText:
        return prepend + generateLocalizedTextCode(node, alloc=asIndirect) + ";"
    elif type(node) == NodeId:
        return prepend + generateNodeIdCode(node) + ";"
    elif type(node) == ExpandedNodeId:
        return prepend + generateExpandedNodeIdCode(node) + ";"
    elif type(node) == DateTime:
        return prepend + generateDateTimeCode(node.value) + ";"
    elif type(node) == QualifiedName:
        return prepend + generateQualifiedNameCode(node.value, alloc=asIndirect) + ";"
    elif type(node) == StatusCode:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == DiagnosticInfo:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == Guid:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == ExtensionObject:
        if asIndirect == False:
            return prepend + "*" + str(instanceName) + ";"
        return prepend + str(instanceName) + ";"
