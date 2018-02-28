from datatypes import *
import datetime
import re

import logging

logger = logging.getLogger(__name__)

def generateBooleanCode(value):
    if value:
        return "true"
    return "false"

def stringtoByteArray(value):
    value = value.strip()
    byteform = [ord(c) for c in value] #transform every char to ASCII
    return byteform

def makeCLiteral(value):
    return value.replace('\\', r'\\\\').replace('\n', r'\\n').replace('\r', r'')

def splitStringLiterals(value, splitLength=500, max_string_length=0):
    """
    Split a string literal longer than splitLength into smaller literals.
    E.g. "Some very long text" will be split into "Some ver" "y long te" "xt"
    On VS2008 there is a maximum allowed length of a single string literal.
    If maxLength is set and the string is longer than maxLength, then an
    empty string will be returned.
    """
    value = value.strip()
    if max_string_length > 0 and len(value) > max_string_length:
        logger.info("String is longer than {}. Returning empty string.".format(max_string_length))
        return "\"\""
    if len(value) < splitLength or splitLength == 0:
        return "\"" + value.replace('"', r'\"') + "\""
    ret = ""
    tmp = value
    while len(tmp) > splitLength:
        ret += "\"" + tmp[:splitLength].replace('"', r'\"') + "\" "
        tmp = tmp[splitLength:]
    ret += "\"" + tmp.replace('"', r'\"') + "\" "
    return ret

def generateStringCode(value, alloc=False, max_string_length=0):
    value = makeCLiteral(value)
    return u"UA_STRING{}({})".format("_ALLOC" if alloc else "", splitStringLiterals(value, max_string_length=max_string_length))

def generateXmlElementCode(value, alloc=False, max_string_length=0):
    value = makeCLiteral(value)
    return u"UA_XMLELEMENT{}({})".format("_ALLOC" if alloc else "", splitStringLiterals(value, max_string_length=max_string_length))

def generateByteStringCode(value):
    asciiarray = stringtoByteArray(value)
    asciiarraystr = str(asciiarray).rstrip(']').lstrip('[')
    return "char stringArr[{}] = {{{}}};\nUA_ByteString variable;\nvariable.length = {};\nvariable.data = (UA_Byte *)stringArr;" \
           "\nif(variable.data == NULL){{\nreturn UA_STATUSCODE_BADOUTOFMEMORY;\n}}"\
                                                .format(len(asciiarray), asciiarraystr, len(asciiarray))

    #here the info is returned in the form we want the code to take. after this line the code will appear just as we wished it to be.


def generateLocalizedTextCode(value, alloc=False, max_string_length=0):
    vt = makeCLiteral(value.text)
    return u"UA_LOCALIZEDTEXT{}(\"{}\", {})".format("_ALLOC" if alloc else "", value.locale,
                                                   splitStringLiterals(vt, max_string_length=max_string_length))

def generateQualifiedNameCode(value, alloc=False, max_string_length=0):
    vn = makeCLiteral(value.name)
    return u"UA_QUALIFIEDNAME{}(ns[{}], {})".format("_ALLOC" if alloc else "",
                                                     str(value.ns), splitStringLiterals(vn, max_string_length=max_string_length))

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

def generateNodeValueCode(prepend , node, instanceName, asIndirect=False, max_string_length=0):
    if type(node) in [Boolean, Byte, SByte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double]:
        return prepend + "(UA_" + node.__class__.__name__ + ") " + str(node.value) + ";"
    elif type(node) == String:
        return prepend + generateStringCode(node.value, alloc=asIndirect, max_string_length=max_string_length) + ";"
    elif type(node) == XmlElement:
        return prepend + generateXmlElementCode(node.value, alloc=asIndirect, max_string_length=max_string_length) + ";"
    elif type(node) == ByteString:
        # replace whitespaces between tags and remove newlines
        return prepend + "UA_BYTESTRING_NULL" if not node.value else generateByteStringCode(re.sub(r">\s*<", "><", re.sub(r"[\r\n]+", \
                                                                                                                          "", node.value)))
        # the replacements done here is just for the array form can be workable in C code. It doesn't couses any problem
        # because the core data used here is already in byte form. So, there is no way we disturb it.
    elif type(node) == LocalizedText:
        return prepend + generateLocalizedTextCode(node, alloc=asIndirect, max_string_length=max_string_length) + ";"
    elif type(node) == NodeId:
        return prepend + generateNodeIdCode(node) + ";"
    elif type(node) == ExpandedNodeId:
        return prepend + generateExpandedNodeIdCode(node) + ";"
    elif type(node) == DateTime:
        return prepend + generateDateTimeCode(node.value) + ";"
    elif type(node) == QualifiedName:
        return prepend + generateQualifiedNameCode(node.value, alloc=asIndirect, max_string_length=max_string_length) + ";"
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
