from datatypes import *
import datetime
import re

def generateBooleanCode(value):
    if value:
        return "true"
    return "false"

def generateStringCode(value, alloc=False):
    return "UA_STRING{}(\"{}\")".format("_ALLOC" if alloc else "", value.replace('"', r'\"'))

def generateXmlElementCode(value, alloc=False):
    return "UA_XMLELEMENT{}(\"{}\")".format("_ALLOC" if alloc else "", value.replace('"', r'\"'))

def generateByteStringCode(value, alloc=False):
    return "UA_BYTESTRING{}(\"{}\")".format("_ALLOC" if alloc else "", value.replace('"', r'\"'))

def generateLocalizedTextCode(value, alloc=False):
    return "UA_LOCALIZEDTEXT{}(\"{}\", \"{}\")".format("_ALLOC" if alloc else "",
                                                       value.locale, value.text.replace('"', r'\"'))

def generateQualifiedNameCode(value, alloc=False):
    return "UA_QUALIFIEDNAME{}(ns{}, \"{}\")".format("_ALLOC" if alloc else "",
                                                     str(value.ns), value.name.replace('"', r'\"'))

def generateNodeIdCode(value):
    if not value:
        return "UA_NODEID_NUMERIC(0,0)"
    if value.i != None:
        return "UA_NODEID_NUMERIC(ns%s,%s)" % (value.ns, value.i)
    elif value.s != None:
        return "UA_NODEID_STRING(ns%s,%s)" % (value.ns, value.s.replace('"', r'\"'))
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateExpandedNodeIdCode(value):
    if value.i != None:
        return "UA_EXPANDEDNODEID_NUMERIC(ns%s, %s)" % (str(value.ns), str(value.i))
    elif value.s != None:
        return "UA_EXPANDEDNODEID_STRING(ns%s, %s)" % (str(value.ns), value.s.replace('"', r'\"'))
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateDateTimeCode(value):
    epoch = datetime.datetime.utcfromtimestamp(0)
    mSecsSinceEpoch = (value - epoch).total_seconds() * 1000.0
    return "( (" + str(mSecsSinceEpoch) + "f * UA_MSEC_TO_DATETIME) + UA_DATETIME_UNIX_EPOCH)"

def generateNodeValueCode(node, instanceName, asIndirect=False):
    if type(node) in [Boolean, Byte, SByte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double]:
        return "(UA_" + node.__class__.__name__ + ") " + str(node.value)
    elif type(node) == String:
        return generateStringCode(node.value, asIndirect)
    elif type(node) == XmlElement:
        return generateXmlElementCode(node.value, asIndirect)
    elif type(node) == ByteString:
        return generateByteStringCode(re.sub(r"[\r\n]+", "", node.value), asIndirect)
    elif type(node) == LocalizedText:
        return generateLocalizedTextCode(node, asIndirect)
    elif type(node) == NodeId:
        return generateNodeIdCode(node)
    elif type(node) == ExpandedNodeId:
        return generateExpandedNodeIdCode(node)
    elif type(node) == DateTime:
        return generateDateTimeCode(node.value)
    elif type(node) == QualifiedName:
        return generateQualifiedNameCode(node.value, asIndirect)
    elif type(node) == StatusCode:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == DiagnosticInfo:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == Guid:
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif type(node) == ExtensionObject:
        if asIndirect == False:
            return "*" + str(instanceName)
        return str(instanceName)
