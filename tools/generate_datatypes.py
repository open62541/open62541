from __future__ import print_function
import sys
import time
import platform
import getpass
from collections import OrderedDict
import re
from lxml import etree
import itertools
import argparse
from pprint import pprint

fixed_size = {"UA_Boolean": 1, "UA_SByte": 1, "UA_Byte": 1, "UA_Int16": 2, "UA_UInt16": 2,
              "UA_Int32": 4, "UA_UInt32": 4, "UA_Int64": 8, "UA_UInt64": 8, "UA_Float": 4,
              "UA_Double": 8, "UA_DateTime": 8, "UA_Guid": 16, "UA_StatusCode": 4}

zero_copy = ["UA_Boolean", "UA_SByte", "UA_Byte", "UA_Int16", "UA_UInt16", "UA_Int32", "UA_UInt32",
             "UA_Int64", "UA_UInt64", "UA_Float", "UA_Double", "UA_DateTime", "UA_StatusCode"]

# The order of the builtin-types is not as in the standard. We put all the
# fixed_size types in the front, so they can be distinguished by a simple geq
# comparison. That's ok, since we use the type-index only internally!!
builtin_types = ["UA_Boolean", "UA_SByte", "UA_Byte", "UA_Int16", "UA_UInt16",
                 "UA_Int32", "UA_UInt32", "UA_Int64", "UA_UInt64", "UA_Float",
                 "UA_Double", "UA_String", "UA_DateTime", "UA_Guid", "UA_ByteString",
                 "UA_XmlElement", "UA_NodeId", "UA_ExpandedNodeId", "UA_StatusCode",
                 "UA_QualifiedName", "UA_LocalizedText", "UA_ExtensionObject", "UA_DataValue",
                 "UA_Variant", "UA_DiagnosticInfo"]

excluded_types = ["UA_NodeIdType", "UA_InstanceNode", "UA_TypeNode", "UA_Node", "UA_ObjectNode",
                  "UA_ObjectTypeNode", "UA_VariableNode", "UA_VariableTypeNode", "UA_ReferenceTypeNode",
                  "UA_MethodNode", "UA_ViewNode", "UA_DataTypeNode", "UA_ServerDiagnosticsSummaryDataType",
                  "UA_SamplingIntervalDiagnosticsDataType", "UA_SessionSecurityDiagnosticsDataType",
                  "UA_SubscriptionDiagnosticsDataType", "UA_SessionDiagnosticsDataType"]

minimal_types = ["InvalidType", "Node", "NodeClass", "ReferenceNode", "ApplicationDescription", "ApplicationType",
                 "ChannelSecurityToken", "OpenSecureChannelRequest", "OpenSecureChannelResponse",
                 "CloseSecureChannelRequest", "CloseSecureChannelResponse", "RequestHeader", "ResponseHeader",
                 "SecurityTokenRequestType", "MessageSecurityMode", "CloseSessionResponse", "CloseSessionRquest",
                 "ActivateSessionRequest", "ActivateSessionResponse", "SignatureData", "SignedSoftwareCertificate",
                 "CreateSessionResponse", "CreateSessionRequest", "EndpointDescription", "UserTokenPolicy", "UserTokenType",
                 "GetEndpointsRequest", "GetEndpointsResponse", "PublishRequest", "PublishResponse", "FindServersRequest", "FindServersResponse",
                 "SetPublishingModeResponse", "SubscriptionAcknowledgement", "NotificationMessage", "ExtensionObject",
                 "Structure", "ReadRequest", "ReadResponse", "ReadValueId", "TimestampsToReturn", "WriteRequest",
                 "WriteResponse", "WriteValue", "SetPublishingModeRequest", "CreateMonitoredItemsResponse",
                 "MonitoredItemCreateResult", "CreateMonitoredItemsRequest", "MonitoredItemCreateRequest",
                 "MonitoringMode", "MonitoringParameters", "TranslateBrowsePathsToNodeIdsRequest",
                 "TranslateBrowsePathsToNodeIdsResponse", "BrowsePath", "BrowsePathResult", "RelativePath",
                 "BrowsePathTarget", "RelativePathElement", "CreateSubscriptionRequest", "CreateSubscriptionResponse",
                 "BrowseResponse", "BrowseResult", "ReferenceDescription", "BrowseRequest", "ViewDescription",
                 "BrowseNextRequest", "BrowseNextResponse", "DeleteSubscriptionsRequest", "DeleteSubscriptionsResponse",
                 "BrowseDescription", "BrowseDirection", "CloseSessionRequest", "AddNodesRequest", "AddNodesResponse",
                 "AddNodesItem", "AddNodesResult", "DeleteNodesItem","AddReferencesRequest", "AddReferencesResponse",
                 "AddReferencesItem","DeleteReferencesItem", "VariableNode", "MethodNode", "VariableTypeNode",
                 "ViewNode", "ReferenceTypeNode", "BrowseResultMask", "ServerState", "ServerStatusDataType", "BuildInfo",
                 "ObjectNode", "DataTypeNode", "ObjectTypeNode", "IdType", "VariableAttributes", "ObjectAttributes",
                 "NodeAttributes","ReferenceTypeAttributes", "ViewAttributes", "ObjectTypeAttributes",
                 "NodeAttributesMask","DeleteNodesItem", "DeleteNodesRequest", "DeleteNodesResponse",
                 "DeleteReferencesItem", "DeleteReferencesRequest", "DeleteReferencesResponse",
                 "RegisterNodesRequest", "RegisterNodesResponse", "UnregisterNodesRequest", "UnregisterNodesResponse", 
                 "UserIdentityToken", "UserNameIdentityToken", "AnonymousIdentityToken", "ServiceFault",
                 "CallMethodRequest", "CallMethodResult", "CallResponse", "CallRequest", "Argument"]

subscription_types = ["DeleteMonitoredItemsRequest", "DeleteMonitoredItemsResponse", "NotificationMessage",
                      "MonitoredItemNotification", "DataChangeNotification", "ModifySubscriptionRequest",
                      "ModifySubscriptionResponse"]

class TypeDescription(object):
    def __init__(self, name, nodeid, namespaceid):
        self.name = name # without the UA_ prefix
        self.nodeid = nodeid
        self.namespaceid = namespaceid

def parseTypeDescriptions(filename, namespaceid):
    definitions = {}
    f = open(filename[0])
    input_str = f.read()
    f.close()
    input_str = input_str.replace('\r','')
    rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))
    for index, row in enumerate(rows):
        if len(row) < 3:
            continue
        if row[2] != "DataType":
            continue
        if row[0] == "BaseDataType":
            definitions["UA_Variant"] = TypeDescription(row[0], row[1], namespaceid)
        elif row[0] == "Structure":
            definitions["UA_ExtensionObject"] = TypeDescription(row[0], row[1], namespaceid)
        else:
            definitions["UA_" + row[0]] = TypeDescription(row[0], row[1], namespaceid)
    return definitions

class BuiltinType(object):
    "Generic type without members. Used for builtin types."
    def __init__(self, name, description = ""):
        self.name = name
        self.description = description

    def fixed_size(self):
        return self.name in fixed_size

    def mem_size(self):
        return fixed_size[self.name]

    def zero_copy(self):
        return self.name in zero_copy

    def typedef_c(self):
        pass

    def typelayout_c(self, namespace_0, description, outname):
        if description == None:
            typeid = "{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}, "
        else:
            typeid = "{.namespaceIndex = %s, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = %s}, " % \
                     (description.namespaceid, description.nodeid)
        if self.name in ["UA_String", "UA_ByteString", "UA_XmlElement"]:
            return "{.typeId = " + typeid + \
                ".memSize = sizeof(" + self.name + "), " + \
                ".namespaceZero = UA_TRUE, .fixedSize = UA_FALSE, .zeroCopyable = UA_FALSE, " + \
                ".membersSize = 1,\n\t.members = {{.memberTypeIndex = UA_TYPES_BYTE, .namespaceZero = UA_TRUE, " + \
                ".padding = offsetof(UA_String, data) - sizeof(UA_Int32), .isArray = UA_TRUE }}, " + \
                ".typeIndex = %s }" % (outname.upper() + "_" + self.name[3:].upper())

        if self.name == "UA_QualifiedName":
            return "{.typeId = " + typeid + \
                ".memSize = sizeof(UA_QualifiedName), " + \
                ".namespaceZero = UA_TRUE, .fixedSize = UA_FALSE, .zeroCopyable = UA_FALSE, " + \
                ".membersSize = 2, .members = {" + \
                "\n\t{.memberTypeIndex = UA_TYPES_UINT16, .namespaceZero = UA_TRUE, " + \
                ".padding = 0, .isArray = UA_FALSE }," + \
                "\n\t{.memberTypeIndex = UA_TYPES_STRING, .namespaceZero = UA_TRUE, " + \
                ".padding = offsetof(UA_QualifiedName, name) - sizeof(UA_UInt16), .isArray = UA_FALSE }},\n" + \
                ".typeIndex = UA_TYPES_QUALIFIEDNAME }"
                
        return "{.typeId = " + typeid + \
            ".memSize = sizeof(" + self.name + "), " + \
            ".namespaceZero = UA_TRUE, " + \
            ".fixedSize = " + ("UA_TRUE" if self.fixed_size() else "UA_FALSE") + \
            ", .zeroCopyable = " + ("UA_TRUE" if self.zero_copy() else "UA_FALSE") + \
            ", .membersSize = 1,\n\t.members = {{.memberTypeIndex = UA_TYPES_" + self.name[3:].upper() + "," + \
            ".namespaceZero = UA_TRUE, .padding = 0, .isArray = UA_FALSE }}, " + \
            ".typeIndex = %s }" % (outname.upper() + "_" + self.name[3:].upper())

class EnumerationType(object):
    def __init__(self, name, description = "", elements = OrderedDict()):
        self.name = name
        self.description = description
        self.elements = elements # maps a name to an integer value

    def append_enum(name, value):
        self.elements[name] = value

    def fixed_size(self):
        return True

    def mem_size(self):
        return 4

    def zero_copy(self):
        return True

    def typedef_c(self):
        if sys.version_info[0] < 3:
            values = self.elements.iteritems()
        else:
            values = self.elements.items()
        return "typedef enum { \n    " + \
            ",\n    ".join(map(lambda kv : kv[0].upper() + " = " + kv[1], values)) + \
            "\n} " + self.name + ";"

    def typelayout_c(self, namespace_0, description, outname):
        if description == None:
            typeid = "{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}, "
        else:
            typeid = "{.namespaceIndex = %s, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = %s}, " % (description.namespaceid, description.nodeid)
        return "{.typeId = " + typeid + \
            ".memSize = sizeof(" + self.name + "), " +\
            ".namespaceZero = UA_TRUE, " + \
            ".fixedSize = UA_TRUE, .zeroCopyable = UA_TRUE, " + \
            ".membersSize = 1,\n\t.members = {{.memberTypeIndex = UA_TYPES_INT32," + \
            ".namespaceZero = UA_TRUE, .padding = 0, .isArray = UA_FALSE }}, .typeIndex = UA_TYPES_INT32 }"

    def functions_c(self, typeTableName):
        return '''#define %s_new (%s*)UA_Int32_new
#define %s_init(p) UA_Int32_init((UA_Int32*)p)
#define %s_delete(p) UA_Int32_delete((UA_Int32*)p)
#define %s_deleteMembers(p) UA_Int32_deleteMembers((UA_Int32*)p)
#define %s_copy(src, dst) UA_Int32_copy((const UA_Int32*)src, (UA_Int32*)dst)
#define %s_encodeBinary(src, dst, offset) UA_Int32_encodeBinary((UA_Int32*)src, dst, offset)
#define %s_decodeBinary(src, offset, dst) UA_Int32_decodeBinary(src, offset, (UA_Int32*)dst)''' % tuple(itertools.repeat(self.name, 8))

class OpaqueType(object):
    def __init__(self, name, description = ""):
        self.name = name
        self.description = description

    def fixed_size(self):
        return False

    def zero_copy(self):
        return False

    def typedef_c(self):
        return "typedef UA_ByteString " + self.name + ";"

    def typelayout_c(self, namespace_0, description, outname):
        if description == None:
            typeid = "{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}, "
        else:
            typeid = "{.namespaceIndex = %s, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = %s}, " % (description.namespaceid, description.nodeid)
        return "{.typeId = " + typeid + \
            ".memSize = sizeof(" + self.name + "), .fixedSize = UA_FALSE, .zeroCopyable = UA_FALSE, " + \
            ".namespaceZero = UA_TRUE, .membersSize = 1,\n\t.members = {{.memberTypeIndex = UA_TYPES_BYTESTRING," + \
            ".namespaceZero = UA_TRUE, .padding = 0, .isArray = UA_FALSE }}, .typeIndex = UA_TYPES_BYTESTRING }"

    def functions_c(self, typeTableName):
        return '''#define %s_new UA_ByteString_new
#define %s_init UA_ByteString_init
#define %s_delete UA_ByteString_delete
#define %s_deleteMembers UA_ByteString_deleteMembers
#define %s_copy UA_ByteString_copy
#define %s_encodeBinary UA_ByteString_encodeBinary
#define %s_decodeBinary UA_ByteString_decodeBinary''' % tuple(itertools.repeat(self.name, 8))

class StructMember(object):
    def __init__(self, name, memberType, isArray):
        self.name = name
        self.memberType = memberType
        self.isArray = isArray

class StructType(object):
    def __init__(self, name, description, members = OrderedDict()):
        self.name = name
        self.description = description
        self.members = members # maps a name to a member definition

    def fixed_size(self):
        for m in self.members.values():
            if m.isArray or not m.memberType.fixed_size():
                return False
        return True

    def mem_size(self):
        total = 0
        for m in self.members.values():
            if m.isArray:
                raise Exception("Arrays have no fixed size!")
            else:
                total += m.memberType.mem_size()
        return total

    def zero_copy(self):
        for m in self.members.values():
            if m.isArray or not m.memberType.zero_copy():
                return False
        return True

    def typedef_c(self):
        if len(self.members) == 0:
            return "typedef void * " + self.name + ";"
        returnstr =  "typedef struct {\n"
        if sys.version_info[0] < 3:
            values = self.members.iteritems()
        else:
            values = self.members.items()
        for name, member in values:
            if member.isArray:
                returnstr += "    UA_Int32 " + name + "Size;\n"
                returnstr += "    " + member.memberType.name + " *" +name + ";\n"
            else:
                returnstr += "    " + member.memberType.name + " " +name + ";\n"
        return returnstr + "} " + self.name + ";"

    def typelayout_c(self, namespace_0, description, outname):
        if description == None:
            typeid = "{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}, "
        else:
            typeid = "{.namespaceIndex = %s, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = %s}, " % (description.namespaceid, description.nodeid)
        layout = "{.typeId = "+ typeid + \
                 ".memSize = sizeof(" + self.name + "), "+ \
                 ".namespaceZero = " + ("UA_TRUE" if namespace_0 else "UA_FALSE") + \
                 ", .fixedSize = " + ("UA_TRUE" if self.fixed_size() else "UA_FALSE") + \
                 ", .zeroCopyable = " + ("sizeof(" + self.name + ") == " + str(self.mem_size()) if self.zero_copy() \
                                         else "UA_FALSE") + \
                 ", .typeIndex = " + outname.upper() + "_" + self.name[3:].upper() + \
                 ", .membersSize = " + str(len(self.members)) + ","
        if len(self.members) > 0:
            layout += "\n\t.members={"
            for index, member in enumerate(self.members.values()):
                layout += "\n\t{" + \
                          ".memberTypeIndex = " + ("UA_TYPES_" + member.memberType.name[3:].upper() if args.namespace_id == 0 or member.memberType.name in existing_types else \
                                                   outname.upper() + "_" + member.memberType.name[3:].upper()) + ", " + \
                          ".namespaceZero = "+ \
                          ("UA_TRUE, " if args.namespace_id == 0 or member.memberType.name in existing_types else "UA_FALSE, ") + \
                          ".padding = "

                before_endpos = "0"
                thispos = "offsetof(%s, %s)" % (self.name, member.name)
                if index > 0:
                    if sys.version_info[0] < 3:
                        before = self.members.values()[index-1]
                    else:
                        before = list(self.members.values())[index-1]
                    before_endpos = "(offsetof(%s, %s)" % (self.name, before.name)
                    if before.isArray:
                        before_endpos += " + sizeof(void*))"
                    else:
                        before_endpos += " + sizeof(%s))" % before.memberType.name
            
                if member.isArray:
                    # the first two bytes are padding for the length index, the last three for the pointer
                    length_pos = "offsetof(%s, %sSize)" % (self.name, member.name)
                    if index != 0:
                        layout += "((%s - %s) << 3) + " % (length_pos, before_endpos)
                    layout += "(%s - sizeof(UA_Int32) - %s)" % (thispos, length_pos)
                else:
                    layout += "%s - %s" % (thispos, before_endpos)
                layout += ", .isArray = " + ("UA_TRUE" if member.isArray else "UA_FALSE") + " }, "
            layout += "}"
        return layout + "}"

    def functions_c(self, typeTableName):
        return '''#define %s_new() (%s*)UA_new(%s)
#define %s_init(p) UA_init(p, %s)
#define %s_delete(p) UA_delete(p, %s)
#define %s_deleteMembers(p) UA_deleteMembers(p, %s)
#define %s_copy(src, dst) UA_copy(src, dst, %s)
#define %s_encodeBinary(src, dst, offset) UA_encodeBinary(src, %s, dst, offset)
#define %s_decodeBinary(src, offset, dst) UA_decodeBinary(src, offset, dst, %s)''' % \
    tuple([self.name] + list(itertools.chain(*itertools.repeat([self.name, "&"+typeTableName+"[" + typeTableName + "_" + self.name[3:].upper()+"]"], 7))))

def parseTypeDefinitions(xmlDescription, existing_types = OrderedDict()):
    '''Returns an ordered dict that maps names to types. The order is such that
       every type depends only on known types. '''
    ns = {"opc": "http://opcfoundation.org/BinarySchema/"}
    tree = etree.parse(xmlDescription)
    typeSnippets = tree.xpath("/opc:TypeDictionary/*[not(self::opc:Import)]", namespaces=ns)
    types = OrderedDict(existing_types.items())

    # types we do not want to autogenerate
    def skipType(name):
        if name in builtin_types:
            return True
        if name in excluded_types:
            return True
        if outname == "ua_types" and not name[3:] in minimal_types:
            return True
        if "Test" in name: # skip all test types
            return True
        if re.search("NodeId$", name) != None:
            return True
        return False

    def stripTypename(tn):
        return tn[tn.find(":")+1:]

    def camlCase2CCase(item):
        "Member names begin with a lower case character"
        return item[:1].lower() + item[1:] if item else ''

    def typeReady(element):
        "Do we have the member types yet?"
        for child in element:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                if stripTypename(child.get("TypeName")) not in types:
                    return False
        return True

    def parseEnumeration(typeXml):	
        name = "UA_" + typeXml.get("Name")
        description = ""
        elements = OrderedDict()
        for child in typeXml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                description = child.text
            if child.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedValue":
                elements[name + "_" + child.get("Name")] = child.get("Value")
        return EnumerationType(name, description, elements)

    def parseOpaque(typeXml):
        name = "UA_" + typeXml.get("Name")
        description = ""
        for child in typeXml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                description = child.text
        return OpaqueType(name, description)

    def parseStructured(typeXml):
        "Returns None if we miss member descriptions"
        name = "UA_" + typeXml.get("Name")
        description = ""
        for child in typeXml:
            if child.tag == "{http://opcfoundation.org/BinarySchema/}Documentation":
                description = child.text
        # ignore lengthfields, just tag the array-members as an array
        lengthfields = []
        for child in typeXml:
            if child.get("LengthField"):
                lengthfields.append(child.get("LengthField"))
        members = OrderedDict()
        for child in typeXml:
            if not child.tag == "{http://opcfoundation.org/BinarySchema/}Field":
                continue
            if child.get("Name") in lengthfields:
                continue
            memberTypeName = "UA_" + stripTypename(child.get("TypeName"))
            if not memberTypeName in types:
                return None
            memberType = types[memberTypeName]
            memberName = camlCase2CCase(child.get("Name"))
            isArray = True if child.get("LengthField") else False
            members[memberName] = StructMember(memberName, memberType, isArray)
        return StructType(name, description, members)

    finished = False
    while(not finished):
        finished = True
        for typeXml in typeSnippets:
            name = "UA_" + typeXml.get("Name")
            if name in types or skipType(name):
                continue
            if typeXml.tag == "{http://opcfoundation.org/BinarySchema/}EnumeratedType":
                t = parseEnumeration(typeXml)
                types[t.name] = t
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}OpaqueType":
                t = parseOpaque(typeXml)
                types[t.name] = t
            elif typeXml.tag == "{http://opcfoundation.org/BinarySchema/}StructuredType":
                t = parseStructured(typeXml)
                if t == None:
                    finished = False
                else:
                    types[t.name] = t

    # remove the existing types
    for k in existing_types.keys():
        types.pop(k)
    return types

parser = argparse.ArgumentParser()
parser.add_argument('--ns0-types-xml', nargs=1, help='xml-definition of the ns0 types that are assumed to already exist')
parser.add_argument('--enable-subscription-types', nargs=1, help='Generate datatypes necessary for Montoring and Subscriptions.')
parser.add_argument('--typedescriptions', nargs=1, help='csv file with type descriptions')
parser.add_argument('namespace_id', type=int, help='the id of the target namespace')
parser.add_argument('types_xml', help='path/to/Opc.Ua.Types.bsd')
parser.add_argument('outfile', help='output file w/o extension')

args = parser.parse_args()
outname = args.outfile.split("/")[-1] 
inname = args.types_xml.split("/")[-1]
existing_types = OrderedDict()
if args.enable_subscription_types:
    minimal_types = minimal_types + subscription_types
if args.namespace_id == 0 or args.ns0_types_xml:
    existing_types = OrderedDict([(t, BuiltinType(t)) for t in builtin_types])
if args.ns0_types_xml:
    if sys.version_info[0] < 3:
        OrderedDict(existing_types.items() + parseTypeDefinitions(args.ns0_types_xml[0], existing_types).items())
    else:
        OrderedDict(list(existing_types.items()) + list(parseTypeDefinitions(args.ns0_types_xml[0], existing_types).items()))
types = parseTypeDefinitions(args.types_xml, existing_types)
if args.namespace_id == 0:
    if sys.version_info[0] < 3:
        types = OrderedDict(existing_types.items() + types.items())
    else:
        types = OrderedDict(list(existing_types.items()) + list(types.items()))

typedescriptions = {}
if args.typedescriptions:
    typedescriptions = parseTypeDescriptions(args.typedescriptions, args.namespace_id)

fh = open(args.outfile + "_generated.h",'w')
fc = open(args.outfile + "_generated.c",'w')
def printh(string):
    print(string, end='\n', file=fh)
def printc(string):
    print(string, end='\n', file=fc)

printh('''/**
* @file ''' + outname + '''_generated.h
*
* @brief Autogenerated data types
*
* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
* on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + '''
*/

#ifndef ''' + outname.upper() + '''_GENERATED_H_
#define ''' + outname.upper() + '''_GENERATED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h" '''
 + ('\n#include "ua_types_generated.h"\n' if args.namespace_id != 0 else '') + '''

/**
* @ingroup types
*
* @defgroup ''' + outname + '''_generated Autogenerated ''' + outname + ''' Types
*
* @brief Data structures that are autogenerated from an XML-Schema.
*
* @{
*/
''')
printh("#define " + outname.upper() + "_COUNT %s\n" % (str(len(types))))
printh("extern UA_EXPORT const UA_DataType *" + outname.upper() + ";\n")

i = 0
if sys.version_info[0] < 3:
    values = types.itervalues()
else:
    values = types.values()

for t in values:
    if type(t) != BuiltinType:
        printh("")
        if t.description != "":
            printh("/** @brief " + t.description + " */")
        printh(t.typedef_c())
    printh("#define " + outname.upper() + "_" + t.name[3:].upper() + " " + str(i))
    if type(t) != BuiltinType:
        printh(t.functions_c(outname.upper()))
    i += 1

printh('''
/// @} /* end of group */\n
#ifdef __cplusplus
} // extern "C"
#endif\n
#endif /* %s_GENERATED_H_ */''' % outname.upper())

printc('''/**
* @file ''' + outname + '''_generated.c
*
* @brief Autogenerated data types
*
* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
* on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + '''
*/\n
#include "stddef.h"
#include "ua_types.h"
#include "''' + outname + '''_generated.h"\n
const UA_DataType *''' + outname.upper() + ''' = (UA_DataType[]){''')
if sys.version_info[0] < 3:
    values = types.itervalues()
else:
    values = types.values()
for t in values:
    printc("")
    printc("/* " + t.name + " */")
    if args.typedescriptions:
        td = typedescriptions[t.name]
    else:
        td = None
    printc(t.typelayout_c(args.namespace_id == 0, td, outname) + ",")
printc("};\n")
# if args.typedescriptions:
#     printc('const UA_UInt32 *' + outname.upper() + '_IDS = (UA_UInt32[]){')
#     for t in types.itervalues():
#         print(str(typedescriptions[t.name].nodeid) + ", ", end='', file=fc)
#     printc("};")

fh.close()
fc.close()
