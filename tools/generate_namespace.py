from __future__ import print_function
import inspect
import sys
import platform
import getpass
import time
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--with-xml', action='store_true', help='generate xml encoding')
parser.add_argument('--with-json', action='store_true', help='generate json encoding')
parser.add_argument('--only-needed', action='store_true', help='generate only types needed for compile')
parser.add_argument('nodeids', help='path/to/NodeIds.csv')
parser.add_argument('outfile', help='outfile w/o extension')
args = parser.parse_args()

# whitelist for "only needed" profile
from type_lists import only_needed_types

# types that are to be excluded
exclude_types = set(["Number", "Integer", "UInteger", "Enumeration", "Image", "ImageBMP",
                     "ImageGIF", "ImageJPG", "ImagePNG", "References", "BaseVariableType",
                     "BaseDataVariableType", "PropertyType", "DataTypeDescriptionType",
                     "DataTypeDictionaryType", "NamingRuleType", "IntegerId", "Counter",
                     "Duration", "NumericRange", "Time", "Date", "UtcTime", "LocaleId",
                     "UserTokenType", "ApplicationType", "ApplicationInstanceCertificate",
                     "ServerVendorCapabilityType", "ServerStatusType",
                     "ServerDiagnosticsSummaryType", "SamplingIntervalDiagnosticsArrayType",
                     "SamplingIntervalDiagnosticsType", "SubscriptionDiagnosticsArrayType",
                     "SubscriptionDiagnosticsType", "SessionDiagnosticsArrayType",
                     "SessionDiagnosticsVariableType", "SessionSecurityDiagnosticsArrayType",
                     "SessionSecurityDiagnosticsType", "DataItemType", "AnalogItemType",
                     "DiscreteItemType", "TwoStateDiscreteType", "MultiStateDiscreteType",
                     "ProgramDiagnosticType", "StateVariableType", "FiniteStateVariableType",
                     "TransitionVariableType", "FiniteTransitionVariableType", "BuildInfoType",
                     "TwoStateVariableType", "ConditionVariableType",
                     "MultiStateValueDiscreteType", "OptionSetType", "ArrayItemType",
                     "YArrayItemType", "XYArrayItemType", "ImageItemType", "CubeItemType",
                     "NDimensionArrayItemType"])

fixed_size = ['UA_DeadbandType', 'UA_DataChangeTrigger', 'UA_Guid', 'UA_ApplicationType',
              'UA_ComplexNumberType', 'UA_EnumeratedTestType', 'UA_BrowseResultMask',
              'UA_TimeZoneDataType', 'UA_NodeClass', 'UA_IdType', 'UA_ServiceCounterDataType',
              'UA_Float', 'UA_ModelChangeStructureVerbMask', 'UA_EndpointConfiguration',
              'UA_NodeAttributesMask', 'UA_DataChangeFilter', 'UA_StatusCode',
              'UA_MonitoringFilterResult', 'UA_OpenFileMode', 'UA_SecurityTokenRequestType',
              'UA_ServerDiagnosticsSummaryDataType', 'UA_ElementOperand',
              'UA_AggregateConfiguration', 'UA_UInt64', 'UA_FilterOperator',
              'UA_ReadRawModifiedDetails', 'UA_ServerState', 'UA_FilterOperand',
              'UA_SubscriptionAcknowledgement', 'UA_AttributeWriteMask', 'UA_SByte', 'UA_Int32',
              'UA_Range', 'UA_Byte', 'UA_TimestampsToReturn', 'UA_UserTokenType', 'UA_Int16',
              'UA_XVType', 'UA_AggregateFilterResult', 'UA_Boolean', 'UA_MessageSecurityMode',
              'UA_AxisScaleEnumeration', 'UA_PerformUpdateType', 'UA_UInt16',
              'UA_NotificationData', 'UA_DoubleComplexNumberType', 'UA_HistoryUpdateType',
              'UA_MonitoringFilter', 'UA_NodeIdType', 'UA_BrowseDirection',
              'UA_SamplingIntervalDiagnosticsDataType', 'UA_UInt32', 'UA_ChannelSecurityToken',
              'UA_RedundancySupport', 'UA_MonitoringMode', 'UA_HistoryReadDetails',
              'UA_ExceptionDeviationFormat', 'UA_ComplianceLevel', 'UA_DateTime', 'UA_Int64',
              'UA_Double']

def useDataType(row):
    if row[0] == "" or row[0] in exclude_types:
        return False
    if row[2] != "DataType":
        return False
    if "Test" in row[0]:
        return False
    if args.only_needed and not(row[0] in only_needed_types):
        return False
    return True

type_names = [] # the names of the datattypes for which we create a vtable entry
def useOtherType(row):
    if row[0] in type_names or row[0] == "":
        return False
    if row[0].startswith("Audit"):
        return False
    if row[0].startswith("Program"):
        return False
    if row[0].startswith("Condition"):
        return False
    if row[0].startswith("Refresh"):
        return False
    if row[0].startswith("OpcUa_"):
        return False
    if row[0].startswith("SessionsDiagnosticsSummaryType_"):
        return False
    if "Type_" in row[0]:
        return False
    if "_Encoding_Default" in row[0]:
        return False
    return True

f = open(args.nodeids)
input_str = f.read() + "\nInvalidType,0,DataType" + "\nHasModelParent,50,ReferenceType"
f.close()
input_str = input_str.replace('\r','')
rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))
for index, row in enumerate(rows):
    if row[0] == "BaseDataType":
    	rows[index]= ("Variant", row[1], row[2])
    elif row[0] == "Structure":
    	rows[index] = ("ExtensionObject", row[1], row[2])

fh = open(args.outfile + ".h",'w')
fc = open(args.outfile + ".c",'w')
def printh(string):
    print(string % inspect.currentframe().f_back.f_locals, end='\n', file=fh)

def printc(string):
    print(string % inspect.currentframe().f_back.f_locals, end='\n', file=fc)

printh('''/**********************************************************
 * '''+args.outfile+'''.hgen -- do not modify
 **********************************************************
 * Generated from '''+args.nodeids+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser()+''' at '''+
       time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/\n 
#ifndef ''' + args.outfile.upper().split("/")[-1] + '''_H_
#define ''' + args.outfile.upper().split("/")[-1] + '''_H_\n
#include "ua_types.h"  // definition of UA_VTable and basic UA_Types
#include "ua_types_generated.h"\n
/**
 * @brief maps namespace zero nodeId to index into UA_VTable
 *
 * @param[in] id The namespace zero nodeId
 *
 * @retval UA_ERR_INVALID_VALUE whenever ns0Id could not be mapped
 * @retval the corresponding index into UA_VTable
 */

UA_Int32 UA_ns0ToVTableIndex(const UA_NodeId *id);\n
extern const UA_TypeVTable UA_EXPORT *UA_TYPES;
extern const UA_NodeId UA_EXPORT *UA_NODEIDS;
extern const UA_ExpandedNodeId UA_EXPORT *UA_EXPANDEDNODEIDS;

/** The entries of UA_TYPES can be accessed with the following indices */ ''')

printc('''/**********************************************************
 * '''+args.outfile.split("/")[-1]+'''.cgen -- do not modify
 **********************************************************
 * Generated from '''+args.nodeids+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser() +
       ''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/\n
#include "''' + args.outfile.split("/")[-1] + '''.h"\n
#include "ua_util.h"
UA_Int32 UA_ns0ToVTableIndex(const UA_NodeId *id) {
	UA_Int32 retval = 0; // InvalidType
        if(id->namespaceIndex != 0) return retval;
	switch (id->identifier.numeric) {''')

i = 0
for index, row in enumerate(rows):
    if not useDataType(row):
        continue
    type_names.append(row[0])
    name = "UA_" + row[0]
    printh("#define "+name.upper()+" "+str(i))
    printh('#define '+name.upper()+'_NS0 '+row[1])
    printc('\tcase '+row[1]+': retval='+name.upper()+'; break; //'+row[2])
    i = i+1
printc('''\t}\n\treturn retval;\n}\n''');

printh('\n#define SIZE_UA_VTABLE '+str(i));
printh("") # newline

printh("/* Now all the non-datatype nodeids */")
for row in rows:
    if not useOtherType(row):
        continue
    name = "UA_" + row[0]
    printh("#define "+name.upper()+" "+str(i))
    printh('#define '+name.upper()+'_NS0 '+row[1])
    i=i+1

printc('''const UA_TypeVTable *UA_TYPES = (UA_TypeVTable[]){''')
for row in rows:
    if row[0] not in type_names:
        continue
    name = "UA_" + row[0]
    printc("\t{.typeId={.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric=" + row[1] + "}" + 
           ",\n.name=(UA_Byte*)&\"%(name)s\"" +
           ",\n.new=(void *(*)())%(name)s_new" +
           ",\n.init=(void(*)(void *))%(name)s_init"+
           ",\n.copy=(UA_Int32(*)(void const * ,void*))%(name)s_copy" +
           ",\n.delete=(void(*)(void *))%(name)s_delete" +
           ",\n.deleteMembers=(void(*)(void *))%(name)s_deleteMembers" +
           ",\n#ifdef UA_DEBUG //FIXME: seems to be okay atm, however a pointer to a noop function would be more safe" + 
           "\n.print=(void(*)(const void *, FILE *))%(name)s_print," +
           "\n#endif" + 
           "\n.memSize=" + ("sizeof(%(name)s)" if (name != "UA_InvalidType") else "0") +
           ",\n.dynMembers=" + ("UA_FALSE" if (name in fixed_size) else "UA_TRUE") +
           ",\n.encodings={{.calcSize=(UA_Int32(*)(const void*))%(name)s_calcSizeBinary" +
           ",\n.encode=(UA_Int32(*)(const void*,UA_ByteString*,UA_UInt32*))%(name)s_encodeBinary" +
           ",\n.decode=(UA_Int32(*)(const UA_ByteString*,UA_UInt32*,void*))%(name)s_decodeBinary}" +
           (",\n{.calcSize=(UA_Int32(*)(const void*))%(name)s_calcSizeXml" +
            ",\n.encode=(UA_Int32(*)(const void*,UA_ByteString*,UA_UInt32*))%(name)s_encodeXml" +
            ",\n.decode=(UA_Int32(*)(const UA_ByteString*,UA_UInt32*,void*))%(name)s_decodeXml}" if (args.with_xml) else "") +
           "}},")

printc('};\n')

# make the nodeids available as well
printc('''const UA_NodeId *UA_NODEIDS = (UA_NodeId[]){''')
for row in rows:
    if not row[0] in type_names:
        continue
    printc("\t{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = " + row[1] + "},")

for row in rows:
    if not useOtherType(row):
        continue
    printc("\t{.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = " + row[1] + "},")
printc('};\n')

# and generate expanded nodeids
printc('''const UA_ExpandedNodeId *UA_EXPANDEDNODEIDS = (UA_ExpandedNodeId[]){''')
for row in rows:
    if not row[0] in type_names:
        continue
    printc("\t{.nodeId = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = " + row[1] +
           "}, .namespaceUri = {.length = -1, .data = UA_NULL}, .serverIndex = 0},")

for row in rows:
    if not useOtherType(row):
        continue
    printc("\t{.nodeId = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = " + row[1] +
           "}, .namespaceUri = {.length = -1, .data = UA_NULL}, .serverIndex = 0},")
printc('};')

printh('\n#endif /* OPCUA_NAMESPACE_0_H_ */')

fh.close()
fc.close()
