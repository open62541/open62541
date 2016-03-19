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
                 "GetEndpointsRequest", "GetEndpointsResponse", "PublishRequest", "PublishResponse", "FindServersRequest",
                 "FindServersResponse", "SetPublishingModeResponse", "SubscriptionAcknowledgement", "NotificationMessage",
                 "ExtensionObject", "Structure", "ReadRequest", "ReadResponse", "ReadValueId", "TimestampsToReturn", "WriteRequest",
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
                 "ObjectNode", "DataTypeNode", "ObjectTypeNode", "IdType", "NodeAttributes",
                 "VariableAttributes", "ObjectAttributes", "ReferenceTypeAttributes", "ViewAttributes", "MethodAttributes",
                 "ObjectTypeAttributes", "VariableTypeAttributes", "DataTypeAttributes", "NodeAttributesMask",
                 "DeleteNodesItem", "DeleteNodesRequest", "DeleteNodesResponse",
                 "DeleteReferencesItem", "DeleteReferencesRequest", "DeleteReferencesResponse",
                 "RegisterNodesRequest", "RegisterNodesResponse", "UnregisterNodesRequest", "UnregisterNodesResponse", 
                 "UserIdentityToken", "UserNameIdentityToken", "AnonymousIdentityToken", "ServiceFault",
                 "CallMethodRequest", "CallMethodResult", "CallResponse", "CallRequest", "Argument",
                 "FilterOperator", "ContentFilterElement", "ContentFilter", "QueryDataDescription",
                 "NodeTypeDescription", "QueryFirstRequest", "QueryDataSet", "ParsingResult",
                 "ContentFilterElementResult", "ContentFilterResult", "QueryFirstResponse",
                 "QueryNextRequest", "QueryNextResponse"]

subscription_types = ["CreateSubscriptionRequest", "CreateSubscriptionResponse",
                      "DeleteMonitoredItemsRequest", "DeleteMonitoredItemsResponse", "NotificationMessage",
                      "MonitoredItemNotification", "DataChangeNotification", "ModifySubscriptionRequest",
                      "ModifySubscriptionResponse", "RepublishRequest", "RepublishResponse"]

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

class Type(object):
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
    
    def functions_c(self, typeTableName):
        return ('''static UA_INLINE void %s_init(%s *p) { memset(p, 0, sizeof(%s)); }
static UA_INLINE void %s_delete(%s *p) { UA_delete(p, %s); }
static UA_INLINE void %s_deleteMembers(%s *p) { ''' + ("UA_deleteMembers(p, &"+typeTableName+"["+typeTableName+"_"+self.name[3:].upper()+"]);" if not self.fixed_size() else "") + ''' }
static UA_INLINE %s * %s_new(void) { return (%s*) UA_new(%s); }
static UA_INLINE UA_StatusCode %s_copy(const %s *src, %s *dst) { ''' + \
                ("*dst = *src; return UA_STATUSCODE_GOOD;" if self.fixed_size() else "return UA_copy(src, dst, &" + typeTableName+"["+typeTableName+"_"+self.name[3:].upper() + "]);") +" }") % \
    tuple([self.name, self.name, self.name] +
          [self.name, self.name, "&"+typeTableName+"[" + typeTableName + "_" + self.name[3:].upper()+"]"] +
          [self.name, self.name] + 
          [self.name, self.name, self.name, "&"+typeTableName+"[" + typeTableName + "_" + self.name[3:].upper()+"]"] +
          [self.name, self.name, self.name]) 

    def encoding_h(self, typeTableName):
        return '''static UA_INLINE UA_StatusCode %s_encodeBinary(const %s *src, UA_ByteString *dst, size_t *offset) { return UA_encodeBinary(src, %s, dst, offset); }
static UA_INLINE UA_StatusCode %s_decodeBinary(const UA_ByteString *src, size_t *offset, %s *dst) { return UA_decodeBinary(src, offset, dst, %s); }''' % \
    tuple(list(itertools.chain(*itertools.repeat([self.name, self.name, "&"+typeTableName+"[" + typeTableName + "_" + self.name[3:].upper()+"]"], 2))))

class BuiltinType(Type):
    "Generic type without members. Used for builtin types."
    def typelayout_c(self, namespace_0, description, outname):
        if description == None:
            typeid = "{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}, "
        else:
            typeid = "{.namespaceIndex = %s, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = %s}, " % \
                     (description.namespaceid, description.nodeid)
        if self.name in ["UA_String", "UA_ByteString", "UA_XmlElement"]:
            return (("{.typeName = \"" + self.name[3:] + "\", ") if typeintrospection else "{") + ".typeId = " + typeid + \
                ".memSize = sizeof(" + self.name + "), " + \
                ".builtin = true, .fixedSize = false, .zeroCopyable = false, " + \
                ".membersSize = 1,\n\t.members = (UA_DataTypeMember[]){{.memberTypeIndex = UA_TYPES_BYTE, .namespaceZero = true, " + \
                (".memberName = \"\", " if typeintrospection else "") + \
                ".padding = 0, .isArray = true }}, " + \
                ".typeIndex = %s }" % (outname.upper() + "_" + self.name[3:].upper())

        if self.name == "UA_QualifiedName":
            return (("{.typeName = \"" + self.name[3:] + "\", ") if typeintrospection else "{") + ".typeId = " + typeid + \
                ".memSize = sizeof(UA_QualifiedName), " + \
                ".builtin = true, .fixedSize = false, .zeroCopyable = false, " + \
                ".membersSize = 2, .members = (UA_DataTypeMember[]){" + \
                "\n\t{.memberTypeIndex = UA_TYPES_UINT16, .namespaceZero = true, " + \
                (".memberName = \"namespaceIndex\", " if typeintrospection else "") + \
                ".padding = 0, .isArray = false }," + \
                "\n\t{.memberTypeIndex = UA_TYPES_STRING, .namespaceZero = true, " + \
                (".memberName = \"name\", " if typeintrospection else "") + \
                ".padding = offsetof(UA_QualifiedName, name)-sizeof(UA_UInt16), .isArray = false }},\n" + \
                ".typeIndex = UA_TYPES_QUALIFIEDNAME }"

        return (("{.typeName = \"" + self.name[3:] + "\", ") if typeintrospection else "{") + ".typeId = " + typeid + \
            ".memSize = sizeof(" + self.name + "), " + \
            ".builtin = true, .fixedSize = " + ("true" if self.fixed_size() else "false") + \
            ", .zeroCopyable = " + ("true" if self.zero_copy() else "false") + \
            ", .membersSize = 1, .members = (UA_DataTypeMember[]){" + \
            "\n\t{.memberTypeIndex = UA_TYPES_" + self.name[3:].upper() + " , .namespaceZero = true, " + \
            (".memberName = \"\", " if typeintrospection else "") + \
            ".padding = 0, .isArray = false }},\n" + \
            ".typeIndex = UA_TYPES_" + self.name[3:].upper() + " }"

class EnumerationType(Type):
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
        return (("{.typeName = \"" + self.name[3:] + "\", ") if typeintrospection else "{") + ".typeId = " + typeid + \
            ".memSize = sizeof(" + self.name + "), .builtin = true, " + \
            ".fixedSize = true, .zeroCopyable = true, " + \
            ".membersSize = 1,\n\t.members = (UA_DataTypeMember[]){{.memberTypeIndex = UA_TYPES_INT32, " + \
            (".memberName = \"\", " if typeintrospection else "") + \
            ".namespaceZero = true, .padding = 0, .isArray = false }}, .typeIndex = UA_TYPES_INT32 }"

class OpaqueType(Type):
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
        return (("{.typeName = \"" + self.name[3:] + "\", ") if typeintrospection else "{") + ".typeId = " + typeid + \
            ".memSize = sizeof(" + self.name + "), .fixedSize = false, .zeroCopyable = false, " + \
            ".builtin = false, .membersSize = 1,\n\t.members = (UA_DataTypeMember[]){{.memberTypeIndex = UA_TYPES_BYTE," + \
            (".memberName = \"\", " if typeintrospection else "") + \
            ".namespaceZero = true, .padding = 0, .isArray = true }}, .typeIndex = %s}" % (outname.upper() + "_" + self.name[3:].upper())

class StructMember(object):
    def __init__(self, name, memberType, isArray):
        self.name = name
        self.memberType = memberType
        self.isArray = isArray

class StructType(Type):
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
                returnstr += "    size_t " + name + "Size;\n"
                returnstr += "    " + member.memberType.name + " *" +name + ";\n"
            else:
                returnstr += "    " + member.memberType.name + " " +name + ";\n"
        return returnstr + "} " + self.name + ";"

    def typelayout_c(self, namespace_0, description, outname):
        if description == None:
            typeid = "{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0}, "
        else:
            typeid = "{.namespaceIndex = %s, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = %s}, " % (description.namespaceid, description.nodeid)
        layout = (("{.typeName = \"" + self.name[3:] + "\", ") if typeintrospection else "{") + ".typeId = " + typeid + \
                 ".memSize = sizeof(" + self.name + "), "+ \
                 ".builtin = false" + \
                 ", .fixedSize = " + ("true" if self.fixed_size() else "false") + \
                 ", .zeroCopyable = " + ("sizeof(" + self.name + ") == " + str(self.mem_size()) if self.zero_copy() \
                                         else "false") + \
                 ", .typeIndex = " + outname.upper() + "_" + self.name[3:].upper() + \
                 ", .membersSize = " + str(len(self.members)) + ","
        if len(self.members) > 0:
            layout += "\n\t.members=(UA_DataTypeMember[]){"
            for index, member in enumerate(self.members.values()):
                layout += "\n\t{" + \
                          ((".memberName = \"" + member.name + "\", ") if typeintrospection else "") + \
                          ".memberTypeIndex = " + ("UA_TYPES_" + member.memberType.name[3:].upper() if args.namespace_id == 0 or member.memberType.name in existing_types else \
                                                   outname.upper() + "_" + member.memberType.name[3:].upper()) + ", " + \
                          ".namespaceZero = "+ \
                          ("true, " if args.namespace_id == 0 or member.memberType.name in existing_types else "false, ") + \
                          ".padding = "

                if not member.isArray:
                    thispos = "offsetof(%s, %s)" % (self.name, member.name)
                else:
                    thispos = "offsetof(%s, %sSize)" % (self.name, member.name)
                before_endpos = "0"
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
                layout += "%s - %s" % (thispos, before_endpos)

                layout += ", .isArray = " + ("true" if member.isArray else "false") + " }, "
            layout += "}"
        return layout + "}"

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
parser.add_argument('--typeintrospection', help='add the type and member names to the idatatype structures', action='store_true')
parser.add_argument('namespace_id', type=int, help='the id of the target namespace')
parser.add_argument('types_xml', help='path/to/Opc.Ua.Types.bsd')
parser.add_argument('outfile', help='output file w/o extension')

args = parser.parse_args()
outname = args.outfile.split("/")[-1] 
inname = args.types_xml.split("/")[-1]
typeintrospection = args.typeintrospection
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
fe = open(args.outfile + "_generated_encoding_binary.h",'w')
fc = open(args.outfile + "_generated.c",'w')
def printh(string):
    print(string, end='\n', file=fh)
def printe(string):
    print(string, end='\n', file=fe)
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

#include "ua_types.h"
#ifdef UA_INTERNAL
#include "ua_types_encoding_binary.h"
#endif'''
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
printh("extern UA_EXPORT const UA_DataType " + outname.upper() + "[];\n")

i = 0
if sys.version_info[0] < 3:
    values = types.itervalues()
else:
    values = types.values()

for t in values:
    printh("")
    if type(t) != BuiltinType:
        if t.description != "":
            printh("/** @brief " + t.description + " */")
        printh(t.typedef_c())
    printh("#define " + outname.upper() + "_" + t.name[3:].upper() + " " + str(i))
    printh(t.functions_c(outname.upper()))
    i += 1

printh('''
/// @} /* end of group */\n
#ifdef __cplusplus
} // extern "C"
#endif\n
#endif /* %s_GENERATED_H_ */''' % outname.upper())

printe('''/**
* @file ''' + outname + '''_generated_encoding_binary.h
*
* @brief Binary encoding for autogenerated data types
*
* Generated from ''' + inname + ''' with script ''' + sys.argv[0] + '''
* on host ''' + platform.uname()[1] + ''' by user ''' + getpass.getuser() + ''' at ''' + time.strftime("%Y-%m-%d %I:%M:%S") + '''
*/\n
#include "ua_types_encoding_binary.h"
#include "''' + outname + '''_generated.h"''')

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
const UA_DataType ''' + outname.upper() + '''[] = {''')
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
    printe("")
    printe("/* " + t.name + " */")
    printe(t.encoding_h(outname.upper()))
printc("};\n")

fh.close()
fe.close()
fc.close()
