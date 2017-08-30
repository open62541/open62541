from datatypes import *

def generateBooleanCode(value):
    if value:
        return "true"
    return "false"

def generateStringCode(value):
    return "UA_STRING(\"" + value + "\")"

def generateXmlElementCode(value):
    return "UA_XMLELEMENT(\"" + value + "\")"

def generateByteStringCode(value):
    return "UA_BYTESTRING(\"" + value + "\")"

def generateLocalizedTextCode(value):
    return "UA_LOCALIZEDTEXT(\"" + value.locale + "\", \"" + value.text + "\")"

def generateQualifiedNameCode(value):
    return "UA_QUALIFIEDNAME(ns" + str(value.ns) + ", \"" + value.name + "\")"

def generateNodeIdCode(value):
    if not value:
        return "UA_NODEID_NUMERIC(0,0)"
    if value.i != None:
        return "UA_NODEID_NUMERIC(ns%s,%s)" % (value.ns, value.i)
    elif value.s != None:
        return "UA_NODEID_STRING(ns%s,%s)" % (value.ns, value.s)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateExpandedNodeIdCode(value):
    if value.i != None:
        return "UA_EXPANDEDNODEID_NUMERIC(ns%s, %s)" % (str(value.ns), str(value.i))
    elif value.s != None:
        return "UA_EXPANDEDNODEID_STRING(ns%s, %s)" % (str(value.ns), value.s)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")
