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
parser.add_argument('nodeids', help='path/to/NodeIds.csv')
parser.add_argument('outfile', help='outfile w/o extension')
args = parser.parse_args()

# types that are to be excluded
exclude_kinds = set(["Object","ObjectType","Variable","Method","ReferenceType"])
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

def skipType(row):
    if row[0] == "" or row[0] in exclude_types or row[2] in exclude_kinds:
        return True
    if "Test" in row[0]:
        return True
    if re.search("Attributes$", row[0]) != None:
        return True
    return False

f = open(args.nodeids)
input_str = f.read() + "\nInvalidType,0,DataType"
input_str = input_str.replace('\r','')
rows = map(lambda x:tuple(x.split(',')), input_str.split('\n'))
f.close()

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
#ifndef ''' + args.outfile.upper() + '''_H_
#define ''' + args.outfile.upper() + '''_H_\n
#include "util/ua_util.h"
#include "ua_types.h"  // definition of UA_VTable and basic UA_Types
#include "ua_types_generated.h"\n
/**
 * @brief maps namespace zero nodeId to index into UA_VTable
 *
 * @param[in] ns0Id The namespace zero nodeId
 *
 * @retval UA_ERR_INVALID_VALUE whenever ns0Id could not be mapped
 * @retval the corresponding index into UA_VTable
 */\n
UA_Int32 UA_ns0ToVTableIndex(const UA_NodeId *id);\n
extern const UA_VTable UA_;\n
static UA_Int32 phantom_delete(void * p) { return UA_SUCCESS; }
extern const UA_VTable UA_borrowed_;\n
/**
 * @brief the set of possible indices into UA_VTable
 *
 * Enumerated type to define the types that the open62541 stack can handle
 */
enum UA_VTableIndex_enum {''')

printc('''/**********************************************************
 * '''+args.outfile+'''.cgen -- do not modify
 **********************************************************
 * Generated from '''+args.nodeids+''' with script '''+sys.argv[0]+'''
 * on host '''+platform.uname()[1]+''' by user '''+getpass.getuser() +
       ''' at '''+ time.strftime("%Y-%m-%d %I:%M:%S")+'''
 **********************************************************/\n
#include "''' + args.outfile + '''.h"\n
UA_Int32 UA_ns0ToVTableIndex(const UA_NodeId *id) {
	UA_Int32 retval = 0; // InvalidType
        if(id->namespace != 0) return retval;
	switch (id->identifier.numeric) {''')

i = 0
for row in rows:
    if skipType(row):
        continue
    if row[0] == "BaseDataType":
    	name = "UA_Variant"
    elif row[0] == "Structure":
        name = "UA_ExtensionObject"
    else:
        name = "UA_" + row[0]
	
    printh("\t"+name.upper()+" = "+str(i)+",")
    printc('\tcase '+row[1]+': retval='+name.upper()+'; break; //'+row[2])
    i = i+1

printh("};\n")
printc('''\n}\nreturn retval;\n}\n''');

printc('''const UA_VTable UA_ = {
\t.getTypeIndex = UA_ns0ToVTableIndex,
\t.types = (UA_VTable_Entry[]){''')

i = 0
for row in rows:
    if skipType(row):
        continue
    if row[0] == "BaseDataType":
        name = "UA_Variant"
    elif row[0] == "Structure":
        name = "UA_ExtensionObject"
    else:
	name = "UA_" + row[0]
	i=i+1
    printh('#define '+name.upper()+'_NS0 '+row[1])

    printc("\t{.typeId={.encodingByte = UA_NODEIDTYPE_TWOBYTE, .namespace = 0,.identifier.numeric=" + row[1] + "}" + 
          ",\n.name=(UA_Byte*)&\"%(name)s\"" +
          ",\n.new=(UA_Int32(*)(void **))%(name)s_new" +
          ",\n.init=(UA_Int32(*)(void *))%(name)s_init"+
          ",\n.copy=(UA_Int32(*)(void const * ,void*))%(name)s_copy" +
          ",\n.delete=(UA_Int32(*)(void *))%(name)s_delete" +
          ",\n.deleteMembers=(UA_Int32(*)(void *))%(name)s_deleteMembers" +
          ",\n#ifdef DEBUG //FIXME: seems to be okay atm, however a pointer to a noop function would be more safe" + 
          "\n.print=(void(*)(const void *, FILE *))%(name)s_print," +
          "\n#endif" + 
          "\n.memSize=" + ("sizeof(%(name)s)" if (name != "UA_InvalidType") else "0") +
          ",\n.dynMembers=" + ("UA_FALSE" if (name in fixed_size) else "UA_TRUE") +
          ",\n.encodings={{.calcSize=(UA_calcSize)%(name)s_calcSizeBinary" +
          ",\n.encode=(UA_encode)%(name)s_encodeBinary" +
          ",\n.decode=(UA_decode)%(name)s_decodeBinary}" +
           (",\n{.calcSize=(UA_calcSize)%(name)s_calcSizeXml" +
            ",\n.encode=(UA_encode)%(name)s_encodeXml" +
            ",\n.decode=(UA_decode)%(name)s_decodeXml}" if (args.with_xml) else "") +
          "}},")

printc('''}};''')

printc("UA_VTable_Entry borrowed_types[" + str(i) + "];");
printc("const UA_VTable UA_borrowed_ = {\n"+
"\t.getTypeIndex=UA_ns0ToVTableIndex,\n"+
"\t.types = borrowed_types\n"+
"};")

printh('\n#define SIZE_UA_VTABLE '+str(i));

printh('\n#endif /* OPCUA_NAMESPACE_0_H_ */')

fh.close()
fc.close()
