from datatypes import  Boolean, Byte, SByte, \
                        Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double, \
                        String, XmlElement, ByteString, Structure, ExtensionObject, LocalizedText, \
                        NodeId, ExpandedNodeId, DateTime, QualifiedName, StatusCode, \
                        DiagnosticInfo, Guid, BuiltinType
import datetime
import re
import string
import logging

logger = logging.getLogger(__name__)

def generateBooleanCode(value):
    if value:
        return "true"
    return "false"

# Strip invalid characters to create valid C identifiers (variable names etc):
def makeCIdentifier(value):
    keywords = frozenset(["double", "int", "float", "char"])
    sanitized = re.sub(r'[^\w]', '', value)
    if sanitized in keywords:
        return "_" + sanitized
    else:
        return sanitized

# Escape C strings:
def makeCLiteral(value):
    mp = []
    for c in range(256):
        if c == ord('\\'): mp.append("\\\\")
        elif c == ord('?'): mp.append("\\?")
        elif c == ord('\''): mp.append("\\'")
        elif c == ord('"'): mp.append("\\\"")
        elif c == ord('\a'): mp.append("\\a")
        elif c == ord('\b'): mp.append("\\b")
        elif c == ord('\f'): mp.append("\\f")
        elif c == ord('\n'): mp.append("\\n")
        elif c == ord('\r'): mp.append("\\r")
        elif c == ord('\t'): mp.append("\\t")
        elif c == ord('\v'): mp.append("\\v")
        elif chr(c) in string.printable: mp.append(chr(c))
        else: mp.append("\\%03o" % c)
    return "".join(mp[c] for c in value.encode('utf-8'))

def splitStringLiterals(value, splitLength=100):
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

def generateLocalizedTextCode(value, alloc=False):
    if value.text is None:
        value.text = ""
    vt = makeCLiteral(value.text)
    return "UA_LOCALIZEDTEXT{}(\"{}\", {})".format("_ALLOC" if alloc else "", '' if value.locale is None else value.locale, splitStringLiterals(vt))

def generateQualifiedNameCode(value, alloc=False,):
    vn = makeCLiteral(value.name)
    return "UA_QUALIFIEDNAME{}(ns[{}], {})".format("_ALLOC" if alloc else "", str(value.ns), splitStringLiterals(vn))

def generateGuidCode(value):
    if isinstance(value, str):
        return f"UA_GUID(\"{value}\")"
    if not value or len(value) != 5:
        return "UA_GUID_NULL"
    else:
        return "UA_GUID(\"{}\")".format('-'.join(value))

def generateNodeIdCode(value):
    if not value:
        return "UA_NODEID_NUMERIC(0, 0)"
    if value.i != None:
        return "UA_NODEID_NUMERIC(ns[{}], {}LU)".format(value.ns, value.i)
    elif value.s != None:
        v = makeCLiteral(value.s)
        return "UA_NODEID_STRING(ns[{}], \"{}\")".format(value.ns, v)
    elif value.g != None:
        return "UA_NODEID_GUID(ns[{}], {})".format(value.ns, generateGuidCode(value.gAsString()))
    raise Exception(str(value) + " NodeID generation for bytestring NodeIDs not supported")

def generateExpandedNodeIdCode(value):
    if value.i != None:
        return "UA_EXPANDEDNODEID_NUMERIC(ns[{}], {}LU)".format(str(value.ns), str(value.i))
    elif value.s != None:
        vs = makeCLiteral(value.s)
        return "UA_EXPANDEDNODEID_STRING(ns[{}], \"{}\")".format(str(value.ns), vs)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")
