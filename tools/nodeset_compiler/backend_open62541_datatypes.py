from datatypes import  Boolean, Byte, SByte, \
                        Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double, \
                        String, XmlElement, ByteString, Structure, ExtensionObject, LocalizedText, \
                        NodeId, ExpandedNodeId, DateTime, QualifiedName, StatusCode, \
                        DiagnosticInfo, Guid, BuiltinType, EnumerationType
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
    keywords = frozenset(["double", "int", "float", "char"])
    sanitized = re.sub(r'[^\w]', '', value)
    if sanitized in keywords:
        return "_" + sanitized
    else:
        return sanitized

# Escape C strings:
def makeCLiteral(value):
    return re.sub(r'(?<!\\)"', r'\\"', value.replace('\\', r'\\').replace('"', r'\"').replace('\n', r'\n').replace('\r', r''))

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
    if value.text is None:
        value.text = ""
    vt = makeCLiteral(value.text)
    return u"UA_LOCALIZEDTEXT{}(\"{}\", {})".format("_ALLOC" if alloc else "", '' if value.locale is None else value.locale,
                                                   splitStringLiterals(vt))

def generateQualifiedNameCode(value, alloc=False,):
    vn = makeCLiteral(value.name)
    return u"UA_QUALIFIEDNAME{}(ns[{}], {})".format("_ALLOC" if alloc else "",
                                                     str(value.ns), splitStringLiterals(vn))

def generateGuidCode(value):
    if isinstance(value, str):
        return "UA_GUID(\"{}\")".format(value)
    if not value or len(value) != 5:
        return "UA_GUID_NULL"
    else:
        return "UA_GUID(\"{}\")".format('-'.join(value))

def generateNodeIdCode(value):
    if not value:
        return "UA_NODEID_NUMERIC(0, 0)"
    if value.i != None:
        return "UA_NODEID_NUMERIC(ns[%s], %sLU)" % (value.ns, value.i)
    elif value.s != None:
        v = makeCLiteral(value.s)
        return u"UA_NODEID_STRING(ns[%s], \"%s\")" % (value.ns, v)
    elif value.g != None:
        return u"UA_NODEID_GUID(ns[%s], %s)" % (value.ns, generateGuidCode(value.gAsString()))
    raise Exception(str(value) + " NodeID generation for bytestring NodeIDs not supported")

def generateExpandedNodeIdCode(value):
    if value.i != None:
        return "UA_EXPANDEDNODEID_NUMERIC(ns[%s], %sLU)" % (str(value.ns), str(value.i))
    elif value.s != None:
        vs = makeCLiteral(value.s)
        return u"UA_EXPANDEDNODEID_STRING(ns[%s], \"%s\")" % (str(value.ns), vs)
    raise Exception(str(value) + " no NodeID generation for bytestring and guid..")

def generateDateTimeCode(value):
    epoch = datetime.datetime.utcfromtimestamp(0)
    mSecsSinceEpoch = int((value - epoch).total_seconds() * 1000.0)
    return "( (UA_DateTime)(" + str(mSecsSinceEpoch) + " * UA_DATETIME_MSEC) + UA_DATETIME_UNIX_EPOCH)"

def lowerFirstChar(inputString):
    return inputString[0].lower() + inputString[1:]

def generateNodeValueCode(prepend , node, instanceName, valueName, global_var_code, asIndirect=False, encRule=None, nodeset=None, idxList=None):
    # TODO: The default values for the remaining data types still have to be added.
    if type(node) in [Boolean, Byte, SByte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double]:
        if node.value is None:
            if type(node) == Boolean:
                node.value = False
            elif type(node) == Double or type(node) == Float:
                node.value = 0.0
            else: 
                node.value = 0
        if encRule is None:
            return prepend + " = (UA_" + node.__class__.__name__ + ") " + str(node.value) + ";"
        else:
            return prepend + " = (UA_" + encRule.member_type.name + ") " + str(node.value) + ";"
    elif isinstance(node, String):
        return prepend + " = " + generateStringCode(node.value, alloc=asIndirect) + ";"
    elif isinstance(node, XmlElement):
        return prepend + " = " + generateXmlElementCode(node.value, alloc=asIndirect) + ";"
    elif isinstance(node, ByteString):
        # Basically the prepend must be passed to the generateByteStrongCode function so that the nested structures are
        # generated correctly. In case of a pointer the valueName is used. This is for example the case with NS0
        # (ns=0;i=8252)
        valueName = valueName if prepend[0] == '*' else prepend
        # replace whitespaces between tags and remove newlines
        return prepend + " = UA_BYTESTRING_NULL;" if not node.value else generateByteStringCode(
            node.value, valueName, global_var_code, isPointer=asIndirect)
        # the replacements done here is just for the array form can be workable in C code. It doesn't couses any problem
        # because the core data used here is already in byte form. So, there is no way we disturb it.
    elif isinstance(node, LocalizedText):
        return prepend + " = " + generateLocalizedTextCode(node, alloc=asIndirect) + ";"
    elif isinstance(node, NodeId):
        return prepend + " = " + generateNodeIdCode(node) + ";"
    elif isinstance(node, ExpandedNodeId):
        return prepend + " = " + generateExpandedNodeIdCode(node) + ";"
    elif isinstance(node, DateTime):
        return prepend + " = " + generateDateTimeCode(node.value) + ";"
    elif isinstance(node, QualifiedName):
        return prepend + " = " + generateQualifiedNameCode(node, alloc=asIndirect) + ";"
    elif isinstance(node, StatusCode):
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif isinstance(node, DiagnosticInfo):
        raise Exception("generateNodeValueCode for type " + node.__class__.name + " not implemented")
    elif isinstance(node, Guid):
        return prepend + " = " + generateGuidCode(node.value) + ";"
    elif isinstance(node, ExtensionObject):
        if asIndirect == False:
            return prepend + " = *" + str(instanceName) + ";"
        return prepend + " = " + str(instanceName) + ";"
    elif isinstance(node, list):
        code = []
        if idxList is None:
            raise Exception("No index was passed and the code generation cannot generate the array element")
        if len(node) == 0:
            return "\n".join(code)
        # Code generation for structure arrays with fields of type Buildin.
        # Example:
        #   Structure []
        #     | | |_ UInt32
        #    | |_ UInt32
        #   |_ UInt16
        if isinstance(encRule.member_type, BuiltinType):
            # Initialize the stack array
            typeOfArray = encRule.member_type.name
            arrayName = encRule.name
            code.append("UA_STACKARRAY(UA_" + typeOfArray + ", " + arrayName+", {0});".format(len(node)))
            # memset is used here instead of UA_Init. Finding the dataType nodeID (to get the type array)
            # would require searching whole nodeset to match the type name
            code.append("memset({arrayName}, 0, sizeof(UA_{typeOfArray}) * {arrayLength});".format(arrayName=arrayName, typeOfArray=typeOfArray, 
                                                                                                   arrayLength=len(node)))
            for idx,subv in enumerate(node):
                code.append(generateNodeValueCode(arrayName + "[" + str(idx) + "]", subv, instanceName, valueName, global_var_code, asIndirect, encRule=encRule, idxList=idx))
            code.append(prepend + "Size = {0};".format(len(node)))
            code.append(prepend + " = " + arrayName +";")
        # Code generation for structure arrays with fields of different types.
        # Example:
        #   Structure []
        #     | |_ String
        #    |_ Structure []
        #          | |_ Double
        #          |_ Double 
        else:
            arrayName = encRule.name
            for idx,subv in enumerate(node):
                encField = encRule.member_type.members[idx].name
                subEncRule = encRule.member_type.members[idx]
                code.append(generateNodeValueCode(arrayName + "[" + str(idxList) + "]" + "." + encField, subv, instanceName, valueName, global_var_code, asIndirect, encRule=subEncRule, idxList=idx))
        return "\n".join(code)
    elif isinstance(node, Structure):
        code = []
        if encRule.is_array:
            if len(node.value) == 0:
                return "\n".join(code)
            # Initialize the stack array
            typeOfArray = encRule.member_type.name
            arrayName = encRule.name
            code.append("UA_STACKARRAY(UA_" + typeOfArray + ", " + arrayName+", {0});".format(len(node.value)))
            # memset is used here instead of UA_Init. Finding the dataType nodeID (to get the type array)
            # would require searching whole nodeset to match the type name
            code.append("memset({arrayName}, 0, sizeof(UA_{typeOfArray}) * {arrayLength});".format(arrayName=arrayName, typeOfArray=typeOfArray, 
                                                                                                   arrayLength=len(node.value)))
            # Values is a list of lists
            # The current index must be passed so that the code path for evaluating lists has the current index value and can generate the code correctly.
            for idx,subv in enumerate(node.value):
                encField = encRule.name
                subEncRule = encRule
                code.append(generateNodeValueCode(prepend + "." + lowerFirstChar(encField), subv, instanceName, valueName, global_var_code, asIndirect, encRule=subEncRule, idxList=idx))
            code.append(prepend + "Size = {0};".format(len(node.value)))
            code.append(prepend + " = " + arrayName +";")

        else:
            for idx,subv in enumerate(node.value):
                encField = encRule.member_type.members[idx].name
                subEncRule = encRule.member_type.members[idx]
                code.append(generateNodeValueCode(prepend + "." + lowerFirstChar(encField), subv, instanceName, valueName, global_var_code, asIndirect, encRule=subEncRule, idxList=idx))

        return "\n".join(code)
